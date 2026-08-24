#pragma once
#include "UnrealEngine/utils.h"

namespace Cheat
{
	bool Init();

	void Uninit();

	void DrawESP();

	// 主循环（每帧由 Present hook 驱动，替代已失效的 PostProcessInput hook）
	void RunTick(const float DeltaTime);
}
