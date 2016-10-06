#pragma once

#include <vector>
#include <memory>

#include "Material\Material.h"
#include "..\Geometry\Primitive.h"
#include "Photon.h"

class RenderGroup {
public:
	std::shared_ptr<Material> material;
	std::vector<Primitive> primitives;
	std::vector<std::vector<Photon>> photons;
	RenderGroup(std::shared_ptr<Material>);
};