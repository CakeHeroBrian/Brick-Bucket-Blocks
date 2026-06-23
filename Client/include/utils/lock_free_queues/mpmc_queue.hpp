#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>

#ifndef __cpp_aligned_new
	#ifdef _WIN32
		#include <malloc.h>
	#else
		#include <stdlib.h>
	#endif
#endif

namespace MPMC {
#if defined(__cpp_lib_hardware_interference_size) && !defined(__APPLE__)
	static constexpr size_t hardwareInterferenceSize = std::hardware_destructive_interference_size;
#else 
	static constexpr size_t hardwareInterferenceSize = 64;
#endif

#if defined(__cpp_aligned_new)
	template <typename T> using AlignedAllocator = std::allocator<T>;

#else

	template <typename T> struct AlignedAllocator {

		T* allocate(size_t n) {
			if (n > std::numeric_limits<size_t>::max() / sizeof(T)) {
				throw std::bad_array_new_length();
			}

#ifdef _WIN32
			auto* ptr = static_cast<T*>(_aligned_malloc(sizeof(T) * n, alignof(T)));
			if (ptr == nullptr) {
				throw std::bad_alloc();
			}
#else
			T* ptr = nullptr;
			if (posix_memalign(reinterpret_cast<void**>(&ptr), alignas(T), sizeof(T) * n) != 0) {
				throw std::bad_alloc();
			}
#endif

			return ptr;
		}

		void deallocate(T* ptr, size_t) {
#ifdef _WIN32
			_aligned_free(ptr);
#else
			free(ptr);
#endif
		}
	};

#endif

	template <typename T>
	struct Slot {
		~Slot() noexcept {
			if (turn & 1) {
				destroy();
			}
		}

		template <typename... Args>
		void construct(Args&& ...args) noexcept {
			static_assert(std::is_nothrow_constructible<T, Args&&...>::value,
				"T must be nothrow constructable with Args&&...");

			::new (static_cast<void*>(&storage)) T(std::forward<Args>(args)...);
		}

		void destroy() noexcept {
			static_assert(std::is_nothrow_destructible<T>::value,
				"T must be nothrow destructable");

			reinterpret_cast<T*>(&storage)->~T();
		}

		T&& move() noexcept { return std::move(*reinterpret_cast<T*>(&storage)); }

		alignas(hardwareInterferenceSize) std::atomic<size_t> turn{ 0 };
		typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
	};

	template <typename T, typename Allocator = AlignedAllocator<Slot<T>>>
	class Queue {
	public:
		explicit Queue(const size_t capacity, const Allocator& allocator = Allocator()) : m_capacity{ capacity }, m_allocator{ allocator }, m_head{ 0 }, m_tail{ 0 } {
			if (m_capacity < 1) {
				throw std::invalid_argument("Capacity must be aleast 1");
			}

			m_slots = m_allocator.allocate(m_capacity);

			if (reinterpret_cast<size_t>(m_slots) % alignof(Slot<T>) != 0) {
				m_allocator.deallocate(m_slots, m_capacity);
				throw std::bad_alloc();
			}

			for (size_t i = 0; i < m_capacity; i++) {
				new (&m_slots[i]) Slot<T>();
			}

			static_assert(
				alignof(Slot<T>) == hardwareInterferenceSize,
				"Slot must be aligned to cache line boundry to prevent false sharing.");

			static_assert(
				sizeof(Slot<T>) % hardwareInterferenceSize == 0,
				"Slot size must be a multiple of cache line size to prevent false sharing between adjacent slots");

			static_assert(
				sizeof(Queue) % hardwareInterferenceSize == 0,
				"Queue size must be a multiple of cache line size to prevent false sharing between adjacent queues");

			//static_assert(
			//	offsetof(Queue, m_tail) - offsetof(Queue, m_head) == static_cast<std::ptrdiff_t>(hardwareInterferenceSize),
			//	"Head and tail must be a cache line apart to prevent false sharing");
		}

		~Queue() noexcept {
			for (size_t i = 0; i < m_capacity; i++) {
				m_slots[i].~Slot();
			}

			m_allocator.deallocate(m_slots, m_capacity);
		}

		Queue(const Queue&) = delete;
		Queue& operator=(const Queue&) = delete;

		Queue(Queue&&) = delete;
		Queue& operator=(Queue&&) = delete;

		template <typename... Args>
		void emplace(Args&& ...args) noexcept {
			static_assert(std::is_nothrow_constructible<T, Args&&...>::value,
				"T must be nothrow constructable with Args&&...");

			auto const head = m_head.fetch_add(1);
			auto& slot = m_slots[index(head)];

			while (turn(head) * 2 != slot.turn.load(std::memory_order_acquire));

			slot.construct(std::forward<Args>(args)...);
			slot.turn.store(turn(head) * 2 + 1, std::memory_order_release);
		}

		template <typename... Args>
		bool try_emplace(Args&& ...args) noexcept {
			static_assert(std::is_nothrow_constructible<T, Args&&...>::value,
				"T must be nothrow constructable with Args&&...");

			auto head = m_head.load(std::memory_order_acquire);

			for (;;) {
				auto& slot = m_slots[index(head)];
				if (turn(head) * 2 == slot.turn.load(std::memory_order_acquire)) {
					if (m_head.compare_exchange_strong(head, head + 1)) {
						slot.construct(std::forward<Args>(args)...);
						slot.turn.store(turn(head) * 2 + 1, std::memory_order_release);
						return true;
					}
				}
				else {
					auto const previousHead = head;
					head = m_head.load(std::memory_order_acquire);
					if (head == previousHead) {
						return false;
					}
				}
			}
		}

		void push(const T& value) noexcept {
			static_assert(
				std::is_nothrow_copy_constructible<T>::value,
				"T must be nothrow copy constructable");

			emplace(value);
		}

		template <typename P, typename = typename std::enable_if<std::is_nothrow_constructible<T, P&&>::value>::type>
		void push(P&& value) noexcept {
			emplace(std::forward<P>(value));
		}

		bool try_push(const T& value) noexcept {
			static_assert(
				std::is_nothrow_copy_constructible<T>::value,
				"T must be nothrow copy constructable");

			return try_emplace(value);
		}

		template <typename P, typename = typename std::enable_if<std::is_nothrow_constructible<T, P&&>::value>::type>
		bool try_push(P&& value) noexcept {
			return try_emplace(value);
		}

		void pop(T& value) noexcept {
			auto const tail = m_tail.fetch_add(1);
			auto& slot = m_slots[index(tail)];

			while (turn(tail) * 2 + 1 != slot.turn.load(std::memory_order_acquire));
			value = slot.move();
			slot.destroy();
			slot.turn.store(turn(tail) * 2 + 2, std::memory_order_release);
		}

		bool try_pop(T& value) noexcept {
			auto tail = m_tail.load(std::memory_order_acquire);

			for (;;) {
				auto& slot = m_slots[index(tail)];
				if (turn(tail) * 2 + 1 == slot.turn.load(std::memory_order_acquire)) {
					if (m_tail.compare_exchange_strong(tail, tail + 1)) {
						value = slot.move();
						slot.destroy();
						slot.turn.store(turn(tail) * 2 + 2, std::memory_order_release);
						return true;
					}
				}
				else {
					auto const previousTail = tail;
					tail = m_tail.load(std::memory_order_acquire);
					if (tail == previousTail) {
						return false;
					}
				}
			}
		}

		ptrdiff_t size() const noexcept {
			return static_cast<ptrdiff_t>(
				m_head.load(std::memory_order_relaxed) - m_tail.load(std::memory_order_relaxed)
			);
		}

		bool empty() const noexcept { return size() <= 0; }
	private:
		static_assert(std::is_nothrow_copy_assignable<T>::value || std::is_nothrow_move_assignable<T>::value,
			"T must be nothrow copy or move assignable");

		static_assert(std::is_nothrow_destructible<T>::value,
			"T must be no throw destructable");
	private:
		constexpr size_t index(size_t i) const noexcept { return i % m_capacity; }
		constexpr size_t turn(size_t i) const noexcept { return i / m_capacity; }
	private:
		const size_t m_capacity = 0;
		Slot<T>* m_slots = nullptr;

#if defined(__has_cpp_attribute) && __has_cpp_attribute(no_unique_address)
		Allocator m_allocator{} [[no_unique_address]];
#else
		Allocator m_allocator{};
#endif

		alignas(hardwareInterferenceSize) std::atomic<size_t> m_head = 0;
		alignas(hardwareInterferenceSize) std::atomic<size_t> m_tail = 0;
	};
}

template <typename T, typename Allocator = MPMC::AlignedAllocator<MPMC::Slot<T>>>
using MPMCQueue = MPMC::Queue<T, Allocator>;