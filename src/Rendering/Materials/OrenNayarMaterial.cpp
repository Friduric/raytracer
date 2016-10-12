#include "OrenNayarMaterial.h"

#include "../../../includes/glm/gtc/constants.hpp"
#include <algorithm>
#include <iostream>


OrenNayarMaterial::OrenNayarMaterial(glm::vec3 color, float _roughness, float _emissivity,
									 float _reflectivity, float _transparency,
									 float _refractiveIndex) :
	surfaceColor(color), roughness(_roughness),
	Material(_emissivity, _reflectivity, _transparency, _refractiveIndex) {}

glm::vec3 OrenNayarMaterial::GetSurfaceColor() const { return surfaceColor; }

glm::vec3 OrenNayarMaterial::CalculateDiffuseLighting(const glm::vec3 & inDirection,
													  const glm::vec3 & outDirection,
													  const glm::vec3 & normal,
													  const glm::vec3 & incomingRadiance) const {

	// See https://en.wikipedia.org/wiki/Oren-Nayar_reflectance_model for more information.

	const float roughnessSquared = roughness * roughness;

	const float A = 1 - 0.5f * roughnessSquared / (roughnessSquared + 0.57f);
	const float B = 0.45f * roughnessSquared / (roughnessSquared + 0.09f);

	const float alphaInclination = acos(glm::dot(normal, -inDirection));
	const float betaInclination = acos(glm::dot(normal, outDirection));

	const float alpha = std::max(alphaInclination, betaInclination);
	const float beta = std::min(alphaInclination, betaInclination);

	// const float angleDifference = glm::dot(outDirection - normal * cosOut, inDirection - normal * cosIn);

	float gamma = glm::dot(-inDirection, outDirection);

	const float oren = A + B * std::max(0.0f, gamma) * sin(alphaInclination) * tan(betaInclination);

	return reflectivity * oren * (incomingRadiance * surfaceColor);
}
