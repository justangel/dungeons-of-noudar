//
// Created by monty on 07/02/17.
//
#include "glm/glm.hpp"

#include <memory>
#include <iostream>
#include <array>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <VBORenderingJob.h>
#include <vector>
#include <utility>
#include <map>
#include <vector>
#include "NativeBitmap.h"
#include "Texture.h"
#include "Material.h"
#include "Trig.h"
#include "TrigBatch.h"
#include "MeshObject.h"
#include "MaterialList.h"
#include "Scene.h"
#include "ETextures.h"
#include "VBORenderingJob.h"
#include "VBORegister.h"
#include "Vec2i.h"
#include "IMapElement.h"
#include "CTeam.h"
#include "CActor.h"
#include "CGameDelegate.h"
#include "CMap.h"
#include "CTile3DProperties.h"
#include "NoudarDungeonSnapshot.h"
#include "RenderingJobSnapshotAdapter.h"


namespace odb {

	glm::mat4 RenderingJobSnapshotAdapter::getSkyTransform(long animationTime) {
		glm::mat4 identity = glm::mat4(1.0f);

		long offset = animationTime;
		int integerPart = offset % ((kSkyTextureLength * 2) * 1000);
		float finalOffset = integerPart / 1000.0f;

		return glm::translate(identity, glm::vec3(finalOffset, 0.0f, 0.0f));
	}

	glm::mat4 RenderingJobSnapshotAdapter::getCubeTransform(glm::vec3 translation) {
		glm::mat4 identity = glm::mat4(1.0f);
		glm::mat4 translated = glm::translate(identity, translation);

		return translated;
	}

	glm::mat4 RenderingJobSnapshotAdapter::getFloorTransform(glm::vec3 translation) {
		glm::mat4 identity = glm::mat4(1.0f);
		glm::mat4 translated = glm::translate(identity, translation);

		return translated;
	}

	void RenderingJobSnapshotAdapter::readSnapshot(const NoudarDungeonSnapshot &snapshot,
	                                               std::map<ETextures, std::vector<VBORenderingJob>> &batches,
	                                               const CTilePropertyMap &tilePropertiesRegistry,
	                                               const std::map<VBORegisterId, VBORegister> &VBORegisters,
	                                               const std::map<std::string, ETextures> &textureRegistry
	) {

		glm::vec3 pos;
		const auto &cubeVBO = VBORegisters.at("cube");
		const auto &billboardVBO = VBORegisters.at("billboard");
		const auto &floorVBO = VBORegisters.at("floor");
		const auto &skyVBO = VBORegisters.at("sky");

		batches.clear();
		batches[ETextures::Skybox].emplace_back(std::get<0>(skyVBO),
		                                        std::get<1>(skyVBO),
		                                        std::get<2>(skyVBO),
		                                        getSkyTransform(snapshot.mTimestamp),
		                                        1.0f, true);

		batches[ETextures::Skybox].emplace_back(std::get<0>(skyVBO),
		                                        std::get<1>(skyVBO),
		                                        std::get<2>(skyVBO),
		                                        getSkyTransform(
				                                        snapshot.mTimestamp + kSkyTextureLength * 1000),
		                                        1.0f, true);

		for (int z = 0; z < Knights::kMapSize; ++z) {
			for (int x = 0; x < Knights::kMapSize; ++x) {

				if (snapshot.mVisibilityMap[z][x] == EVisibility::kInvisible) {
					continue;
				}

				auto tile = snapshot.map[z][x];
				auto actor = snapshot.snapshot[z][x];
				int splatFrame = snapshot.splat[z][x];

				Shade shade = (0.25f * std::min(255, snapshot.mLightMap[z][x]) / 255.0f) + 0.75f;

				if (x == static_cast<int>( snapshot.mCursorPosition.x) &&
				    z == static_cast<int>( snapshot.mCursorPosition.y)) {
					shade = 1.5f;
				}

				if (tilePropertiesRegistry.count(tile) <= 0) {
					continue;
				}

				auto tileProperties = tilePropertiesRegistry.at(tile);


				if (tileProperties.mCeilingTexture != mNullTexture) {
					pos = glm::vec3(x * 2, -5.0f + (2.0 * tileProperties.mCeilingHeight), z * 2);
					batches[textureRegistry.at(tileProperties.mCeilingTexture)].emplace_back(std::get<0>(floorVBO),
					                                                                         std::get<1>(floorVBO),
					                                                                         std::get<2>(floorVBO),
					                                                                         getFloorTransform(pos),
					                                                                         shade, true);
				}

				if (tileProperties.mCeilingRepeatedWallTexture != mNullTexture) {

					const auto &tileVBO = VBORegisters.at(tileProperties.mVBOToRender);

					for (float y = 0; y < tileProperties.mCeilingRepetitions; ++y) {

						pos = glm::vec3(x * 2,
						                -4.0f + (2.0f * tileProperties.mCeilingHeight) + (2.0 * y),
						                z * 2);

						batches[textureRegistry.at(tileProperties.mCeilingRepeatedWallTexture)].emplace_back(
								std::get<0>(tileVBO),
								std::get<1>(tileVBO),
								std::get<2>(tileVBO),
								getCubeTransform(pos),
								shade, true);
					}
				}

				if (tileProperties.mMainWallTexture != mNullTexture) {
					const auto &tileVBO = VBORegisters.at(tileProperties.mVBOToRender);

					pos = glm::vec3(x * 2, -4.0f, z * 2);

					batches[textureRegistry.at(tileProperties.mMainWallTexture)].emplace_back(
							std::get<0>(tileVBO),
							std::get<1>(tileVBO),
							std::get<2>(tileVBO),
							getCubeTransform(pos), shade, true);
				}

				if (tileProperties.mFloorRepeatedWallTexture != mNullTexture) {

					const auto &tileVBO = VBORegisters.at(tileProperties.mVBOToRender);

					for (float y = 0; y < tileProperties.mFloorRepetitions; ++y) {

						//the final -1.0f in y is for accounting fore the block's length
						pos = glm::vec3(x * 2,
						                -5.0f + (2.0f * tileProperties.mFloorHeight) - (2.0 * y) -
						                1.0f, z * 2);

						batches[textureRegistry.at(tileProperties.mFloorRepeatedWallTexture)].emplace_back(
								std::get<0>(tileVBO),
								std::get<1>(tileVBO),
								std::get<2>(tileVBO),
								getCubeTransform(pos),
								shade, true);
					}
				}

				if (tileProperties.mFloorTexture != mNullTexture) {
					pos = glm::vec3(x * 2, -5.0f + (2.0f * tileProperties.mFloorHeight), z * 2);
					batches[textureRegistry.at(tileProperties.mFloorTexture)].emplace_back(std::get<0>(floorVBO),
					                                                                       std::get<1>(floorVBO),
					                                                                       std::get<2>(floorVBO),
					                                                                       getFloorTransform(pos),
					                                                                       shade, true);
				}
			}
		}
	}
}
