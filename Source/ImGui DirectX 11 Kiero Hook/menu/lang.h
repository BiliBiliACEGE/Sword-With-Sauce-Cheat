#pragma once

// ============ 双语言字符串定义 ============
// 中文版：编译时定义 SWS_CN（构建配置 ReleaseCN）
// 英文版：默认（构建配置 ReleaseEN）
// 所有菜单 UI 字符串集中于此，新增文案时两处同步添加

#ifdef SWS_CN

// ============ 中文 ============
#define TR_TITLE				u8"Sword With Sauce 2.4.0 | 作弊菜单"

// ESP 区域
#define TR_ESP_SECTION			u8"透视功能"
#define TR_ESP_BOXES			u8"敌人边框"
#define TR_ESP_BOX_2D			u8"2D边框"
#define TR_ESP_BOX_3D			u8"3D边框"
#define TR_ESP_BOX_CORNER		u8"角点边框"
#define TR_ESP_BOX_TYPE			u8"边框类型"
#define TR_ESP_SNAPLINES		u8"透视线"
#define TR_ESP_SKELETON			u8"敌人骨架"

// 触发器区域
#define TR_TRG_SECTION			u8"敌人触发器"
#define TR_TRG_ENABLE			u8"启用"
#define TR_TRG_HOTKEY			u8"热键"
#define TR_TRG_DELAY			u8"间隔"
#define TR_TRG_DELAY_MODE		u8"间隔模式"
#define TR_TRG_DELAY_ITEMS		u8"自定义\0随机\0\0"
#define TR_TRG_DELAY_FORMAT		u8"%d 毫秒"
#define TR_TRG_DELAY_MIN		u8"最小 Delay"
#define TR_TRG_DELAY_MAX		u8"最大 Delay"

// 实用功能区域
#define TR_UTIL_SECTION			u8"实用功能"
#define TR_AIMBOT				u8"自动锁敌"
#define TR_AIMBOT_VIS_CHECK		u8"可见度检查"
#define TR_AIMBOT_FOV			u8"视野"
#define TR_AIMBOT_FOV_CIRCLE	u8"视野范围"
#define TR_AIMBOT_SMOOTH		u8"视角平滑"

// 作弊功能
#define TR_EXP_INF_HEALTH		u8"无限生命"
#define TR_EXP_INF_AMMO			u8"无限弹药"
#define TR_EXP_KILL_ALL			u8"消灭群体"
#define TR_EXP_KILL_ALL_TIP		u8"勾选杀死全部存活敌人"
#define TR_EXP_NO_CLIP			u8"穿墙"
#define TR_EXP_FLY				u8"飞行模式"
#define TR_EXP_SPEED			u8"修改速度"
#define TR_EXP_SUPER_JUMP		u8"超级跳跃"
#define TR_EXP_GRAVITY			u8"修改重力"

// 通用
#define TR_HOTKEY_RESET_TIP		u8"双击重置"

#else

// ============ English ============
#define TR_TITLE				"Sword With Sauce 2.4.0 | Cheat Menu"

// ESP section
#define TR_ESP_SECTION			"ESP"
#define TR_ESP_BOXES			"Enemy Boxes"
#define TR_ESP_BOX_2D			"2D Box"
#define TR_ESP_BOX_3D			"3D Box"
#define TR_ESP_BOX_CORNER		"Corner Box"
#define TR_ESP_BOX_TYPE			"Box Type"
#define TR_ESP_SNAPLINES		"Snaplines"
#define TR_ESP_SKELETON			"Skeleton"

// Triggerbot section
#define TR_TRG_SECTION			"Triggerbot"
#define TR_TRG_ENABLE			"Enable"
#define TR_TRG_HOTKEY			"Hotkey"
#define TR_TRG_DELAY			"Delay"
#define TR_TRG_DELAY_MODE		"Delay Mode"
#define TR_TRG_DELAY_ITEMS		"Custom\0Random\0\0"
#define TR_TRG_DELAY_FORMAT		"%d ms"
#define TR_TRG_DELAY_MIN		"Min Delay"
#define TR_TRG_DELAY_MAX		"Max Delay"

// Utilities section
#define TR_UTIL_SECTION			"Utilities"
#define TR_AIMBOT				"Aimbot"
#define TR_AIMBOT_VIS_CHECK		"Visibility Check"
#define TR_AIMBOT_FOV			"FOV"
#define TR_AIMBOT_FOV_CIRCLE	"FOV Circle"
#define TR_AIMBOT_SMOOTH		"Smoothing"

// Exploits
#define TR_EXP_INF_HEALTH		"Infinite Health"
#define TR_EXP_INF_AMMO			"Infinite Ammo"
#define TR_EXP_KILL_ALL			"Kill All"
#define TR_EXP_KILL_ALL_TIP		"Kill all alive enemies on toggle"
#define TR_EXP_NO_CLIP			"No Clip"
#define TR_EXP_FLY				"Fly Mode"
#define TR_EXP_SPEED			"Speed Hack"
#define TR_EXP_SUPER_JUMP		"Super Jump"
#define TR_EXP_GRAVITY			"Gravity Mod"

// Common
#define TR_HOTKEY_RESET_TIP		"Double-click to reset"

#endif
