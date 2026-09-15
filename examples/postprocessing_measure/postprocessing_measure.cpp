#include <imgui.h>
#include <VVPPL.h>

import std;
import VEEngine;

/// Measurement copy of the postprocessing example: no input, no GUI, fixed camera.
// Usage: VVPP_CHAIN=<direct|empty|full> postprocessing_measure <warmup> <frames> [screenshot.png]
namespace {
constexpr auto crateTextureRelativePath = "assets/game/crate0/diffuse.png";	

/// @brief Finds the repository-style asset root from either the cwd or executable location.
[[nodiscard]] std::filesystem::path assetRoot(char *argv0) {
	auto containsGameAssets = [](const std::filesystem::path &candidate) {
		return std::filesystem::exists(candidate / crateTextureRelativePath);
	};
	if (const auto cwd = std::filesystem::current_path(); containsGameAssets(cwd)) {
		return cwd;
	}
	if (argv0 == nullptr) {
		return {};
	}
	auto executable = std::filesystem::absolute(std::filesystem::path{argv0});
	if (std::filesystem::exists(executable)) {
		executable = std::filesystem::weakly_canonical(executable);
	}
	for (auto candidate = executable.parent_path(); !candidate.empty(); candidate = candidate.parent_path()) {
		if (containsGameAssets(candidate)) {
			return candidate;
		}
		if (candidate == candidate.root_path()) {
			break;
		}
	}
	return {};
}

/// @brief Adds the game floor and three crate cubes through facade scene authoring calls.
[[nodiscard]] std::expected<void, vve::Error> loadGameScene(vve::RenderSystem render, const std::filesystem::path &root) {
	constexpr vve::Vec3 cubeMinimum{-0.5F, -0.5F, -0.5F}; ///< Unit cube lower corner.
	constexpr vve::Vec3 cubeMaximum{0.5F, 0.5F, 0.5F};    ///< Unit cube upper corner.
	constexpr float cubeCenterY = 0.5F;                   ///< Unit cube bottom sits on the y=0 ground plane.
	const auto crateTexture = root / crateTextureRelativePath; ///< Crate diffuse texture.

	render.clearScene();
	if (auto result = render.addPlane(vve::Vec2{6.0F, 4.0F}, vve::LinearColor{.value = vve::Vec3{0.1F, 0.6F, 0.2F}});
		 !result) {
		return std::unexpected(result.error());
	}
	for (const vve::Vec3 center : std::array{vve::Vec3{-1.5F, cubeCenterY, -0.5F},
														  vve::Vec3{0.0F, cubeCenterY, 0.75F},
														  vve::Vec3{1.5F, cubeCenterY, -0.5F}}) {
		if (auto result = render.addTexturedCuboid(cubeMinimum, cubeMaximum, crateTexture,
																 vve::Transform{.translation = vve::Position{.value = center}});
			 !result) {
			return std::unexpected(result.error());
		}
	}
	return {};
}

} // namespace

int main(int argc, char **argv) {
	// Warm-up frames, timed frames and an optional screenshot path from the command line
	if (argc < 3) {
		std::cerr << "usage: postprocessing_measure <warmup> <frames> [screenshot.png]\n";
		return 1;
	}
	const int warmup = std::stoi(argv[1]);
	const int frames = std::stoi(argv[2]);

	// Chain from the environment: direct copy without the library, empty chain or full chain
	const char *chainValue = std::getenv("VVPP_CHAIN");
	const std::string chain = chainValue != nullptr ? chainValue : "";
	if (chain != "direct" && chain != "empty" && chain != "full") {
		std::cerr << "VVPP_CHAIN must be direct, empty or full\n";
		return 1;
	}

	const auto activeRenderer = vve::RendererId{.value = "forward"}; ///< Renderer id selected through the facade.
	auto engine = vve::EngineBuilder<>{}
						 .applicationName("postprocessing_measure")
						 .addWindow(vve::WindowSetup{}
										 .id("main")
										 .title("VVE Post Processing Measurement")
										 .extent(vve::PixelExtent{.width = 960, .height = 540})
										 .renderer(activeRenderer)
										 .resizable(false))
						 .build();

	if (const auto result = engine.init(); !result) {
		std::cerr << "[postprocessing] engine init failed: error=" << vve::errorName(result.error()) << '\n';
		return 1;
	}

	auto render = engine.world().get<vve::RenderSystem>();

	// Without a setup the renderer creates no library instance and copies the image itself
	if (chain == "empty") {
		render.setPostProcessSetup([](vvppl::PostProcessing &) {});
	}

	// Same effects and order as in the postprocessing example and in bench, with the default settings
	if (chain == "full") {
		render.setPostProcessSetup([](vvppl::PostProcessing &pp) {
			pp.addTonemap();
			pp.addChromatic();
			pp.addGreyscale();
			pp.addVignette();
			pp.addFilmGrain();
			pp.addColorGrade();
			pp.addSolarize();
			pp.addSabattier();
			pp.addEmboss();
			pp.addSobel();
			pp.addSpeedLines();
			pp.addHighlight();
			pp.addSegmentation();
			pp.addDither();
		});
	}


	if (const auto result = loadGameScene(render, assetRoot(argc > 0 ? argv[0] : nullptr)); !result) {
		std::cerr << "[postprocessing] scene load failed: error=" << vve::errorName(result.error()) << '\n';
		return 2;
	}

	// Simple Lights
	const auto white = vve::LinearColor{.value = vve::Vec3{1.0F, 1.0F, 1.0F}};
	const auto ambient = vve::LinearColor{.value = vve::Vec3{0.05F, 0.05F, 0.05F}};
	render.setDirectionalLight(vve::Direction{.value = vve::Vec3{-0.5F, -1.0F, 0.5F}}, white,
							vve::LightIntensity{.value = 1.0F}, ambient);
	render.setPointLight(vve::Position{.value = vve::Vec3{2.0F, 4.0F, 2.0F}}, white,
						vve::LightIntensity{.value = 3.0F}, vve::LightRange{.value = 8.0F}, ambient);

	// Camera fixed at the start position of the postprocessing example
	render.setCamera(vve::Camera::lookAt(vve::Position{.value = vve::Vec3{-2.0F, 1.5F, 6.0F}},
										 vve::Position{.value = vve::Vec3{0.0F, 1.0F, 0.0F}}),
					 vve::PixelExtent{.width = 960, .height = 540});

	
	// Warm-up frames, not timed
	for (int i = 0; i < warmup; ++i) {
		const auto status = engine.step();
		if (!status || *status == vve::FrameStatus::stopped) {
			std::cerr << "[postprocessing_measure] frame failed during warm-up\n";
			return 3;
		}
	}

	// Timed frames
	const auto start = std::chrono::steady_clock::now();
	for (int i = 0; i < frames; ++i) {
		const auto status = engine.step();
		if (!status || *status == vve::FrameStatus::stopped) {
			std::cerr << "[postprocessing_measure] frame failed\n";
			return 3;
		}
	}
	const auto end = std::chrono::steady_clock::now();
	const double meanMs = std::chrono::duration<double, std::milli>(end - start).count() / frames;

	// Screenshot after the timing, because it renders an extra frame and waits for the GPU
	if (argc > 3) {
		if (const auto result = render.captureFrameToPng(argv[3]); !result) {
			std::cerr << "[postprocessing_measure] screenshot failed: error=" << vve::errorName(result.error()) << '\n';
			return 4;
		}
	}

	std::cout << chain << ',' << warmup << ',' << frames << ',' << meanMs << '\n';
	return 0;
}
