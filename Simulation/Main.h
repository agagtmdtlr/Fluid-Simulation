#pragma once


class Main : public IExecute
{
	// Inherited via IExecute
public: virtual void Initialize() override;
public: virtual void Ready() override;
public: virtual void Destroy() override;
public: virtual void Update() override;
public: virtual void PreRender() override;
public: virtual void Render() override ;
public: virtual void PostRender() override;
public: virtual void ResizeScreen() override;

private: void Push(IExecute* execute);

private: vector<IExecute*> executes;
};

