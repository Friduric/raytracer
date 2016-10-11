#include "Octree.h"

#include <queue>
#include <iostream>

#include "../../includes/glm/gtx/norm.hpp"

Octree::~Octree() {
	DeleteRecursive(&root);
}

void Octree::DeleteRecursive(OctreeNode* node) {
	if (node->IsLeaf()) {
		delete node;
	}
	else {
		for (unsigned int i = 0; i < OctreeNode::CHILDREN_PER_NODE; ++i) {
			DeleteRecursive(node->children[i]);
		}
	}
}

void Octree::SetUpOctree(const Scene & scene, const unsigned int maxPhotonsPerNode, const float maxSizeOfNodeBox) {
	SetUpRootNode(scene);
	std::queue<OctreeNode*> nodeQueue;
	nodeQueue.push(&root);
	float nodeWidth, nodeHeight, nodeDepth;
	float nodeXHalf, nodeYHalf, nodeZHalf;
	float nodeXMin, nodeYMin, nodeZMin;
	float nodeXMax, nodeYMax, nodeZMax;

	// While there are nodes left in the queue divide into 8 subnodes.
	while (!nodeQueue.empty()) {
		OctreeNode* currentNode = nodeQueue.front();
		nodeQueue.pop();
		nodeWidth = currentNode->xMax - currentNode->xMin;
		nodeHeight = currentNode->yMax - currentNode->yMin;
		nodeDepth = currentNode->zMax - currentNode->zMin;

		// If the amount of photons in the node is larger than/equal to maxPhotonsPerNode,
		// and if all sides of the node box is larger than maxSizeOfNodeBox,
		// then split the node into 8 new nodes.
		if (currentNode->photons.size() >= maxPhotonsPerNode || maxSizeOfNodeBox <= nodeWidth ||
			maxSizeOfNodeBox <= nodeHeight || maxSizeOfNodeBox <= nodeDepth) {
			nodeXHalf = 0.5f * nodeWidth;
			nodeYHalf = 0.5f * nodeHeight;
			nodeZHalf = 0.5f * nodeDepth;
			unsigned int idxCounter = 0;
			for (unsigned int xIdx = 0; xIdx < 2; ++xIdx) {
				nodeXMin = currentNode->xMin + nodeXHalf*xIdx;
				nodeXMax = nodeXMin + nodeXHalf;
				for (unsigned int yIdx = 0; yIdx < 2; ++yIdx) {
					nodeYMin = currentNode->yMin + nodeYHalf*yIdx;
					nodeYMax = nodeYMin + nodeYHalf;
					for (unsigned int zIdx = 0; zIdx < 2; ++zIdx) {
						nodeZMin = currentNode->zMin + nodeZHalf*zIdx;
						nodeZMax = nodeZMin + nodeZHalf;
						// Add new node						
						OctreeNode* newNode = CreateNode(currentNode->photons, nodeXMin, nodeXMax,
														 nodeYMin, nodeYMax, nodeZMin, nodeZMax);
						currentNode->children[idxCounter] = newNode;
						newNode->parent = currentNode;
						nodeQueue.push(newNode);
						idxCounter++;
					}
				}
			}
		}
	}
}

void Octree::SetUpRootNode(const Scene & scene) {
	root.xMin = scene.xMin; root.xMax = scene.xMax;
	root.yMin = scene.yMin; root.yMax = scene.yMax;
	root.zMin = scene.zMin; root.zMax = scene.zMax;
	// Add all photons to the root	
	for (unsigned int n = 0; n < photons.size(); ++n) {
		root.photons.push_back(&photons[n]);
	}
}

Octree::OctreeNode* Octree::CreateNode(const std::vector<Photon*> & photons,
									   const float xMin, const float xMax,
									   const float yMin, const float yMax,
									   const float zMin, const float zMax) {
	OctreeNode* node = new OctreeNode();
	// Set min and max values.
	node->xMin = xMin; node->xMax = xMax;
	node->yMin = yMin; node->yMax = yMax;
	node->zMin = zMin; node->zMax = zMax;
	// Find and add all photons inside the box of this node.
	for (unsigned int i = 0; i < photons.size(); ++i) {
		glm::vec3 pos = photons[i]->position;
		if (pos.x >= xMin && pos.x <= xMax &&
			pos.y >= yMin && pos.y <= yMax &&
			pos.z >= zMin && pos.z <= zMax) {
			node->photons.push_back(photons[i]);
		}
	}
	return node;
}

Octree::OctreeNode* Octree::GetNodeClosestToPosition(const glm::vec3 & pos) {
	OctreeNode* bestNode = &root;
	// Search as long as we're not at a leaf and find the closest node to the given position.
	while (!bestNode->IsLeaf()) {
		float closestDistance = FLT_MAX;
		OctreeNode* closestNode = bestNode->children[0];
		for (unsigned int i = 0; i < OctreeNode::CHILDREN_PER_NODE; ++i) {
			OctreeNode* tmpNode = bestNode->children[i];
			glm::vec3 tmpPos = glm::vec3(tmpNode->xMin + 0.5f * (tmpNode->xMax - tmpNode->xMin),
										 tmpNode->yMin + 0.5f * (tmpNode->yMax - tmpNode->yMin),
										 tmpNode->zMin + 0.5f * (tmpNode->zMax - tmpNode->zMin));
			float tmpDist = glm::distance2(tmpPos, pos);
			if (tmpDist < closestDistance) {
				closestDistance = tmpDist;
				closestNode = bestNode->children[i];
			}
		}
		bestNode = closestNode;
	}
	return bestNode;
}

bool Octree::OctreeNode::IsLeaf() const {
	return children[0] == NULL;
}
