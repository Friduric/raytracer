#pragma once

#include <glm.hpp>

class Material {
public:
	virtual bool IsEmissive() const = 0;
	virtual glm::vec3 GetSurfaceColor() const = 0;
	virtual glm::vec3 GetEmissionColor() const = 0;

	/// <summary> 
	/// Calculates the BRDF value of this material. 
	/// In other words calculates the outgoing radiance. 
	/// </summary>
	/// <param name='inDirection'> The incoming direction of the light.</param>
	/// <param name='outDirection'> The outgoing direction of the light.</param>
	/// <param name='normal'> The normal of the surface.</param>
	/// <param name='incomingIntensity'> The intensity of the incoming light.</param>
	/// <param name='surfaceColor'> The color of the surface.</param>
	/// <returns> The BRDF value of this material. </returns>
	virtual glm::vec3 CalculateBRDF(const glm::vec3 & inDirection, const glm::vec3 & outDirection,
									const glm::vec3 & normal, const glm::vec3 & incomingIntensity) const = 0;
};
