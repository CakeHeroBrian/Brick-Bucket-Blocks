#include <print>
#include <chrono>

#include <core/window.hpp>

#include <rendering/texture_manager.hpp>

#include <rendering/Renderer.hpp>

#include <camera/camera.hpp>
#include <core/input.hpp>

#include <game/world/world.hpp>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <game/gameplay/player/Player.hpp>

#include <game/gameplay/player/raycasting.hpp>

#include <game/world/blocks/block.hpp>
#include <game/world/world.hpp>
#include <game/world/Region.hpp>

void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum serverity,
	GLsizei length, const GLchar* message, const void* userParam) {
	if (id == 131169 || id == 131185 || id == 131204 || id == 131218) return;

	std::println("---------------");
	std::println("Debug Message ({}): {}", id, message);

	switch (source) {
	case GL_DEBUG_SOURCE_API:
		std::println("Source: API");
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		std::println("Source: Window System");
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		std::println("Source: Shader Compiler");
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		std::println("Source: Third Party");
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		std::println("Source: Application");
		break;
	case GL_DEBUG_SOURCE_OTHER:
		std::println("Source: Other");
		break;
	}

	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		std::println("Type: Error");
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		std::println("Type: Depricated Behavior");
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		std::println("Type: Undefined Behavior");
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		std::println("Type: Portability");
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		std::println("Type: Performance");
		break;
	case GL_DEBUG_TYPE_MARKER:
		std::println("Type: Marker");
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		std::println("Type: Push Group");
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		std::println("Type: Pop Group");
		break;
	case GL_DEBUG_TYPE_OTHER:
		std::println("Type: Other");
		break;
	}

	switch (serverity) {
	case GL_DEBUG_SEVERITY_HIGH:
		std::println("Serverity: High");
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		std::println("Serverity: Medium");
		break;
	case GL_DEBUG_SEVERITY_LOW:
		std::println("Serverity: Low");
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		std::println("Serverity: Notification");
		break;
	}

	std::println("");
}

enum class FrustumOption : uint8_t {
	Outside = 0,
	Inside = 1,
	Intersects = 2
};

static FrustumOption isChunkInFrustum(const glm::vec3& min, const glm::vec3& max, const std::array<glm::vec4, 6>& planes);
uint32_t vao = 0;

int main() {
	if (!glfwInit()) {
		std::println("Failed to initialize GLFW");
		return -1;
	}

	Window::CreateInfo windowCreateInfo{};
	windowCreateInfo.width = 1920;
	windowCreateInfo.height = 1080;
	windowCreateInfo.title = "Brick Bucket";
	windowCreateInfo.preCreationWindowHints = { 
		{GLFW_CONTEXT_VERSION_MAJOR, 4},
		{GLFW_CONTEXT_VERSION_MINOR, 6},
		{GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE}, 
		{GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE} 
	};

	Window window = Window(windowCreateInfo);

	window.makeContextCurrent();
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::println("Failed to initialize OpenGL.");
	}

	GLint flags = 0;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}


	window.setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	Renderer::CreateInfo rendererCreateInfo{};
	rendererCreateInfo.pWindow = &window;
	

	Renderer renderer(rendererCreateInfo); 

	ShaderProgram& shaderProgram = renderer.getShaderProgram();
	ShaderProgram& crosshairShaderProgram = renderer.getCrosshairShaderProgram();

	TextureManager::CreateInfo textureManagerCreateInfo{};
	textureManagerCreateInfo.textureWidth = 16;
	textureManagerCreateInfo.textureHeight = 16;
	textureManagerCreateInfo.textureLayers = 256;
	textureManagerCreateInfo.textureFolder = "assets/textures/";

	TextureManager textureManager(textureManagerCreateInfo);

	textureManager.addTexture("missing");
	textureManager.addTexture("grass");
	textureManager.addTexture("red_wool");
	textureManager.addTexture("grass_side");
	textureManager.addTexture("log_side");
	textureManager.addTexture("stone");
	textureManager.addTexture("leaves");
	textureManager.addTexture("dirt");
	textureManager.addTexture("water");
	textureManager.addTexture("sand");
	textureManager.generateMipMaps();


	World::BlockInfo airInfo{};
	airInfo.blockName = "air";
	airInfo.type = Block::Type::Air;
	airInfo.sideData = {

	};

	World::BlockInfo grassInfo{};
	grassInfo.blockName = "grass";
	grassInfo.type = Block::Type::Solid;
	grassInfo.sideData = {
		{ "grass_side", Block::Side::Sides }, 
		{ "grass", Block::Side::Top }, 
		{ "dirt", Block::Side::Bottom }
	};

	World::BlockInfo stoneInfo{};
	stoneInfo.blockName = "stone";
	stoneInfo.type = Block::Type::Solid;
	stoneInfo.sideData = {
		{"stone", Block::Side::All}
	};

	World::BlockInfo dirtInfo{};
	dirtInfo.blockName = "dirt";
	dirtInfo.type = Block::Type::Solid;
	dirtInfo.sideData = {
		{"dirt", Block::Side::All}
	};

	World::BlockInfo redWoolInfo{};
	redWoolInfo.blockName = "red_wool";
	redWoolInfo.type = Block::Type::Solid;
	redWoolInfo.sideData = {
		{"red_wool", Block::Side::All}
	};

	World::BlockInfo waterInfo{};
	waterInfo.blockName = "water";
	waterInfo.type = Block::Type::Water;
	waterInfo.sideData = {
		{"water", Block::Side::All}
	};

	World::BlockInfo sandInfo{};
	sandInfo.blockName = "sand";
	sandInfo.type = Block::Type::Solid;
	sandInfo.sideData = {
		{"sand", Block::Side::All}
	};

	World::CreateInfo worldCreateInfo{.textureManager = textureManager };
	worldCreateInfo.blockCreateInfos = {
		airInfo,
		grassInfo,
		stoneInfo,
		dirtInfo,
		redWoolInfo,
		waterInfo,
		sandInfo
	};
	World world(worldCreateInfo);

	auto previousForTime = std::chrono::high_resolution_clock::now();
	float deltaTime = 1.0f / 60.0f;

	Player::CreateInfo playerCreateInfo{};
	playerCreateInfo.position = glm::vec3(0.0f, 80.0f, 5.0f);
	playerCreateInfo.fov = 70.0f;
	playerCreateInfo.nearPlane = 0.01f;
	playerCreateInfo.farPlane = 1024.0f;

	Player player(playerCreateInfo);

	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glEnableVertexArrayAttrib(vao, 0);
	glEnableVertexArrayAttrib(vao, 1);

	glVertexArrayAttribIFormat(
		vao,
		0,
		1,
		GL_UNSIGNED_INT,
		offsetof(VertexData, faceData)
	);
	
	glVertexArrayAttribIFormat(
		vao,
		1,
		1,
		GL_UNSIGNED_INT,
		offsetof(VertexData, skylightData)
	);

	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribBinding(vao, 1, 0);

	glVertexArrayBindingDivisor(vao, 0, 1);

	uint32_t crosshairVao = 0;
	glCreateVertexArrays(1, &crosshairVao);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	auto tickPrevious = std::chrono::high_resolution_clock::now();

	while (!window.shouldClose()) {
		glfwPollEvents();
		Input::captureEvents();

		glClearColor(0.5f, 0.8f, 0.9f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (Input::isKeyDown(GLFW_KEY_ESCAPE)) {
			break;
		}

		if (Input::isKeyDown(GLFW_KEY_R)) {
			player.resetVelocity();
			glm::vec3 playerPosition = player.getPosition();
			player.setPosition(glm::vec3(playerPosition.x, world.getSurfaceAt(playerPosition.x, playerPosition.z) + 2.0f, playerPosition.z));
		}

		glBindVertexArray(vao);
		world.processChunkUploads(vao);

		shaderProgram.bind();

		auto currentForTime = std::chrono::high_resolution_clock::now();
		deltaTime = std::chrono::duration<float>(currentForTime - previousForTime).count();
		previousForTime = currentForTime;

		bool mouseHeld = Input::isMouseButtonDown(GLFW_MOUSE_BUTTON_2);
		bool mousePressed = Input::isMouseButtonPressed(GLFW_MOUSE_BUTTON_2);

		auto tickCurrent = std::chrono::high_resolution_clock::now();
		auto deltaTick = std::chrono::duration<float>(tickCurrent - tickPrevious).count();
		if (deltaTick >= 1.0f / 20.0f) {
			player.tickUpdate(deltaTick, world);
			world.update(player.getPosition());

			bool brokeBlock = false;
			if (Input::isMouseButtonDown(GLFW_MOUSE_BUTTON_1) && player.canBreakBlocks()) {
				player.breakBlock(world);
				player.applyBreakBlockCooldown();
				brokeBlock = true;
			}

			bool placedBlock = false;
			if (mousePressed) {
				const Block& placeholderHoldenBlock = world.getBlock("stone");
				player.placeBlock(placeholderHoldenBlock, world);
				player.startPlaceCharge();
				placedBlock = true;
			}
			else if (mouseHeld && player.canPlaceBlock()) {
				const Block& placeholderHoldenBlock = world.getBlock("stone");
				player.placeBlock(placeholderHoldenBlock, world);
				player.applyPlaceBlockCooldown();
				placedBlock = true;
			}
			else if (!mouseHeld) {
				player.resetPlaceCharge();
			}

			if (!brokeBlock) {
				player.decrementBreakBlockCooldown();
			}
			
			if (!placedBlock) {
				player.decrementPlaceBlockCooldown();
			}

			mousePressed = false;

			tickPrevious += std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(
				std::chrono::duration<float>(1.0f / 20.0f)
			);
		}
		
		auto sinceLastTick = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - tickPrevious).count();
		float alpha = sinceLastTick / (1.0f / 20.0f);

		player.interpolate(alpha);
		player.update(deltaTime);

		const glm::mat4& viewMatrix = player.getViewMatrix();
		const glm::mat4& projectionMatrix = player.getPerspectiveMatrix(window.getAspectRatio());

		const glm::mat4 projectionViewMatrix = projectionMatrix * viewMatrix;

		shaderProgram.bind();
		glBindVertexArray(vao);
		shaderProgram.uploadMat4("uProjectionView", projectionViewMatrix);
		shaderProgram.uploadVec3("uViewPosition", player.getRenderPosition());
		shaderProgram.uploadVec4("fogColor", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
		shaderProgram.uploadFloat("fogStart", 400.0f);
		shaderProgram.uploadFloat("fogEnd", 920.0f);

		glm::mat4 clip = glm::transpose(projectionViewMatrix);

		std::array<glm::vec4, 6> planes{};
		glm::vec4 scratch{ 0.0f };

		// Left   plane
		scratch = clip[3] + clip[0];
		planes[0] = scratch / glm::length(glm::vec3(scratch));

		// Right  plane
		scratch = clip[3] - clip[0];
		planes[1] = scratch / glm::length(glm::vec3(scratch));

		// Bottom plane
		scratch = clip[3] + clip[1];
		planes[2] = scratch / glm::length(glm::vec3(scratch));

		// Top plane
		scratch = clip[3] - clip[1];
		planes[3] = scratch / glm::length(glm::vec3(scratch));

		// Near plane
		scratch = clip[3] + clip[2];
		planes[4] = scratch / glm::length(glm::vec3(scratch));

		// Far plane
		scratch = clip[3] - clip[2];
		planes[5] = scratch / glm::length(glm::vec3(scratch));

		textureManager.bind(0);

		gtl::vector<Chunk*> visibleChunks{};
		for (const auto& [_, region] : world.getRegions()) {
			FrustumOption regionResult = isChunkInFrustum(region->bbMin, region->bbMax, planes);
			if (regionResult == FrustumOption::Outside) {
				continue;
			}

			for (const auto& chunkAtomic : region->chunks) {
				Chunk* chunk = chunkAtomic.load(std::memory_order_acquire);
				if (!chunk || chunk->getDrawCount() == 0 || chunk->isDying() ) {
					continue;
				}

				if (regionResult == FrustumOption::Intersects) {
					if (isChunkInFrustum(chunk->getBBMin(), chunk->getBBMax(), planes) == FrustumOption::Outside) {
						continue;
					}
				}

				visibleChunks.emplace_back(chunk);
			}
		}

		world.sortChunks(player.getPosition(), visibleChunks);

		glBindVertexArray(vao);

		for (const auto& chunk : visibleChunks) {
			shaderProgram.uploadMat4("uTransform", chunk->getChunkTransfrom());

			glVertexArrayVertexBuffer(
				vao,
				0,
				chunk->getDrawBufferId(),
				0,
				sizeof(VertexData)
			);

			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, chunk->getDrawCommandId());
			glDrawArraysIndirect(GL_TRIANGLES, nullptr);
		}

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		for (const auto& chunk : visibleChunks) {
			shaderProgram.uploadMat4("uTransform", chunk->getChunkTransfrom());

			glVertexArrayVertexBuffer(
				vao,
				0,
				chunk->getTransparentDrawBufferId(),
				0,
				sizeof(VertexData)
			);

			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, chunk->getTransparentDrawCommandId());
			glDrawArraysIndirect(GL_TRIANGLES, nullptr);
		}

		glDisable(GL_BLEND);

		// draw crosshair

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);

		crosshairShaderProgram.bind();

		glViewport(0, 0, window.getWidth(), window.getHeight());

		glBindVertexArray(crosshairVao);

		crosshairShaderProgram.uploadFloat("uAspectRatio", window.getAspectRatio());

		glLineWidth(2.0f);
		glDrawArrays(GL_LINES, 0, 4);
		glLineWidth(1.0f);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		

		window.swapBuffers();
		Input::endFrame();
	}

	glfwTerminate();
}

FrustumOption isChunkInFrustum(const glm::vec3& min, const glm::vec3& max, const std::array<glm::vec4, 6>& planes) {
	glm::vec3 realMin = glm::min(min, max);
	glm::vec3 realMax = glm::max(min, max);

	int32_t totalInside = 0;

	for (const auto& plane : planes) {
		glm::vec3 positiveVertex = realMin;
		glm::vec3 negativeVertex = realMax;

		if (plane.x >= 0) {
			positiveVertex.x = realMax.x;
			negativeVertex.x = realMin.x;
		}

		if (plane.y >= 0) {
			positiveVertex.y = realMax.y;
			negativeVertex.y = realMin.y;
		}

		if (plane.z >= 0) {
			positiveVertex.z = realMax.z;
			negativeVertex.z = realMin.z;
		}


		if (glm::dot(glm::vec3(plane), positiveVertex) + plane.w < -0.01f)
			return FrustumOption::Outside;

		if (glm::dot(glm::vec3(plane), negativeVertex) + plane.w >= 0.0f)
			totalInside++;
	}

	return (totalInside == 6) ? FrustumOption::Inside : FrustumOption::Intersects;
}