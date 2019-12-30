#pragma once
#include "pch.h"
#include "Scene.h"


class SceneLoader
{
	Array<Scene*> sceneArr;

	void initializeGeometryFromMeshes(Scene* scene, const Array<Mesh*>& meshes);
	void computeModelMatrices(Scene* scene);

public:
	Scene* getScene(uint sceneIdx) const { return sceneArr[sceneIdx]; }
	Scene* push_testScene1();
	Scene* push_hyperionTestScene();
};
