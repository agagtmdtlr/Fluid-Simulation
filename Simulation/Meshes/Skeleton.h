#pragma once

extern const UINT BONE_MAX_SIZE = 255;


struct Bone
{
	wstring name;
	UINT parent;
};

class Skeleton
{

public:
public:
	Skeleton(Shader * shader, wstring file);
	virtual ~Skeleton();

private:
	Bone mBones[BONE_MAX_SIZE];
};


class SkeletonResourceManager
{
	friend class Skeleton;

public:
	static void Load(wstring file);
public:
	SkeletonResourceManager();
	virtual ~SkeletonResourceManager();
};




