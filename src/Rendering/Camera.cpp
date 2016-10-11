#include "Camera.h"

#include <iostream>
#include <fstream>
#include <random>
#include <algorithm>
#include <chrono>

#include "../Geometry/Ray.h"
#include "../Utility/Math.h"

#define __LOG_ITERATIONS

Camera::Camera(const int _width, const int _height) : width(_width), height(_height) {
	pixels.assign(width, std::vector<Pixel>(height));
	discretizedPixels.assign(width, std::vector<glm::u8vec3>(height));
}

void Camera::Render(const Scene & scene, const RenderingMode RENDERING_MODE, const unsigned int RAYS_PER_PIXEL,
					const unsigned int RAY_MAX_DEPTH, const unsigned int RAY_MAX_BOUNCE,
					const glm::vec3 eye, const glm::vec3 c1, const glm::vec3 c2,
					const glm::vec3 c3, const glm::vec3 c4) {
	if (RENDERING_MODE != RenderingMode::MONTE_CARLO) {
		if (scene.photonMap == nullptr) {
			std::cerr << "The photon map for the scene has not yet been initialized! ";
			std::cerr << "Impossible to render using the photon map." << std::endl;
			return;
		}
	}

	std::cout << "Rendering the scene ..." << std::endl;
	auto startTime = std::chrono::high_resolution_clock::now();

	/* Initialize random engines. */
	std::random_device rd;
	std::default_random_engine gen(rd());
	std::uniform_real_distribution<float> rand(0, 1.0f - FLT_EPSILON);

	/* Precompute inv width and heights. */
	const float invWidth = 1.0f / (float)width;
	const float invHeight = 1.0f / (float)height;
	const float INV_RAYS_PER_PIXEL = 1.0f / static_cast<float>(RAYS_PER_PIXEL);

	/* Step lengths. */
	const unsigned int SQRT_QUADS_PER_PIXEL = (unsigned int)(sqrt(RAYS_PER_PIXEL) + 0.5f); // Round.
	const float INV_SQRT_QUADS_PER_PIXEL = 1.0f / (float)SQRT_QUADS_PER_PIXEL;
	const float cstep = invWidth * INV_SQRT_QUADS_PER_PIXEL;
	const float rstep = invHeight * INV_SQRT_QUADS_PER_PIXEL;

	/* Initialize ray components. */
	Ray ray;
	glm::vec3 colorAccumulator;
	const glm::vec3 cameraPlaneNormal = -glm::normalize(glm::cross(c1 - c2, c1 - c4));
#ifdef __LOG_ITERATIONS
	long long ctr = 0;
#endif // __LOG_ITERATIONS
	/* For each pixel, shoot a bunch of rays through it. */
	for (unsigned int y = 0; y < width; ++y) {
		for (unsigned int z = 0; z < height; ++z) {
#ifdef __LOG_ITERATIONS
			if (++ctr % 10000 == 0) {
				std::cout << ctr << "/" << width * height << " pixels." << std::endl;
			}
#endif // __LOG_ITERATIONS	
			/* Shoot a bunch of rays through the pixel (y, z), and accumulate color. */
			colorAccumulator = glm::vec3(0, 0, 0);
			for (float c = 0; c < invWidth; c += cstep) {
				for (float r = 0; r < invHeight; r += rstep) {

					/* Calculate new randomized point in the camera plane. */
					const float ylerp = (y + c + rand(gen) * INV_SQRT_QUADS_PER_PIXEL) * invWidth;
					const float zlerp = (z + r + rand(gen) * INV_SQRT_QUADS_PER_PIXEL) * invHeight;
					const float nx = Math::BilinearInterpolation(ylerp, zlerp, c1.x, c2.x, c3.x, c4.x);
					const float ny = Math::BilinearInterpolation(ylerp, zlerp, c1.y, c2.y, c3.y, c4.y);
					const float nz = Math::BilinearInterpolation(ylerp, zlerp, c1.z, c2.z, c3.z, c4.z);

					/* Create ray. */
					ray.from = glm::vec3(nx, ny, nz);
					ray.dir = glm::normalize(ray.from - eye);
					const float rayFactor = std::max(0.0f, glm::dot(ray.from, cameraPlaneNormal));

					/* Choose render mode. */
					switch (RENDERING_MODE) {
					case RenderingMode::MONTE_CARLO:
						/* Trace ray through the scene. */
						colorAccumulator += rayFactor * scene.TraceRay(ray, RAY_MAX_BOUNCE, RAY_MAX_DEPTH);
						break;
					case RenderingMode::MONTE_CARLO_USING_PHOTON_MAP:
						/* Trace a ray through the scene. */
						colorAccumulator += rayFactor * scene.TraceRayUsingPhotonMap(ray, RAY_MAX_BOUNCE, RAY_MAX_DEPTH);
						break;
					case RenderingMode::VISUALIZE_PHOTON_MAP:
						/* Shoot a ray through the scene and sample using the photon map. */
						unsigned int intersectionRenderGroupIndex, intersectionPrimitiveIndex;
						float intersectionDistance;
						if (scene.RayCast(ray, intersectionRenderGroupIndex, intersectionPrimitiveIndex, intersectionDistance)) {
							const auto & photons = scene.photonMap->GetPhotonsInOctreeNodeOfPosition(ray.from + intersectionDistance * ray.dir);
							for (const Photon * p : photons) {
								colorAccumulator += rayFactor * p->color;
							}
						}
						break;
					}
				}
			}
			/* Set pixel color dependent on ray trace. */
			pixels[y][z].color = INV_RAYS_PER_PIXEL * colorAccumulator;
		}
	}

	auto endTime = std::chrono::high_resolution_clock::now();

	auto took = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
	std::cout << "Rendering finished and took: " << (took / 1000.0) << " seconds." << std::endl;

	/* Create the final discretized image from float the values. */
	CreateImage(); // Should always be done immediately after the rendering step.
}

void Camera::CreateImage(const float BRIGHTNESS_DISCRETIZATION_THRESHOLD) {
	std::cout << "Creating a discretized image from the rendered image ..." << std::endl;

	/* Find max color intensity. */
	float maxIntensity = 0;
	for (size_t i = 0; i < width; ++i) {
		for (size_t j = 0; j < height; ++j) {
			const auto & c = pixels[i][j].color;
			maxIntensity = std::max<float>(c.r, maxIntensity);
			maxIntensity = std::max<float>(c.g, maxIntensity);
			maxIntensity = std::max<float>(c.b, maxIntensity);
		}
	}

	// TODO: Change this to some other way of detecting whether the image is dark (with spots).
	// if (maxIntensity > BRIGHTNESS_DISCRETIZATION_THRESHOLD) {
		// std::cout << "Squashing brightness color due to high brightness." << std::endl;
	for (size_t i = 0; i < width; ++i) {
		for (size_t j = 0; j < height; ++j) {
			pixels[i][j].color = sqrt(pixels[i][j].color);
		}
	}
	maxIntensity = sqrt(maxIntensity);
	// }

	if (maxIntensity < FLT_EPSILON * 4.0f) {
		std::cerr << "Rendered image intensity was very low. Impossible to discretize image." << std::endl;
		return;
	}

	// Discretize pixels using the max intensity. Every value must be between 0 and 255.
	glm::u8 discretizedMaxIntensity{};
	const float f = 254.99f / maxIntensity;
	for (size_t i = 0; i < width; ++i) {
		for (size_t j = 0; j < height; ++j) {
			const auto & c = f * pixels[i][j].color;
			assert(c.r >= -FLT_EPSILON && c.r <= 255.5f - FLT_EPSILON);
			assert(c.g >= -FLT_EPSILON && c.g <= 255.5f - FLT_EPSILON);
			assert(c.b >= -FLT_EPSILON && c.b <= 255.5f - FLT_EPSILON);
			discretizedPixels[i][j].r = (glm::u8)round(c.r);
			discretizedPixels[i][j].g = (glm::u8)round(c.g);
			discretizedPixels[i][j].b = (glm::u8)round(c.b);
			discretizedMaxIntensity = glm::max(discretizedMaxIntensity, discretizedPixels[i][j].r);
			discretizedMaxIntensity = glm::max(discretizedMaxIntensity, discretizedPixels[i][j].g);
			discretizedMaxIntensity = glm::max(discretizedMaxIntensity, discretizedPixels[i][j].b);
		}
	}
	assert(discretizedMaxIntensity == 255); // Discretized max intensity failed! Image max intensity is OK.
	std::cout << "Image max intensity was: " << maxIntensity << std::endl;
}

bool Camera::WriteImageToTGA(const std::string path) const {
	std::cout << "Writing image to TGA ..." << std::endl;

	assert(width > 0 && height > 0);

	std::ofstream o(path.c_str(), std::ios::out | std::ios::binary);

	// Write header.
	const std::string header = "002000000000";
	for (unsigned int i = 0; i < header.length(); ++i) {
		o.put(header[i] - '0');
	}

	o.put(width & 0x00FF);
	o.put((width & 0xFF00) >> 8);
	o.put(width & 0x00FF);
	o.put((height & 0xFF00) >> 8);
	o.put(32); // 24 bit bitmap.
	o.put(0);

	// Write data.
	for (unsigned int y = 0; y < height; ++y) {
		for (unsigned int x = 0; x < width; ++x) {
			auto& cp = discretizedPixels[x][y];
			o.put(cp.b);
			o.put(cp.g);
			o.put(cp.r);
			o.put((char)0xFF); // 255.
		}
	}

	o.close();

	return true;
}