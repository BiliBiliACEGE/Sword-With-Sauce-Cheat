# UE4 作弊源代码和指南
**仅供教育用途**

这是一个为Sword With Sauce（一款UE4游戏）制作的小型作弊菜单，我花了大约一周时间完成。我启动这个项目的主要目标是学习虚幻引擎游戏黑客的基础知识。

**如果你觉得这个仓库有任何用处，请给它点个star，让更多人看到！**

在这个仓库中你会找到：
* 带注释的UE4多功能作弊源代码，包含常见游戏黑客技术如函数调用、钩子和通过模式扫描自动更新
* UE4游戏黑客的入门和深入指南

![alt text](https://github.com/1hAck-0/UE4-Cheat-Source-Code/raw/master/Media/Menu.png?raw=true)

## 功能
* ESP
  * 方框
    * 2D方框
    * 3D方框
    * 角框
  * 瞄准线
  * 骨骼

* 自动瞄准
  * 可见性检查
  * 视野范围+FOV圆圈
  * 平滑度
  * 自定义热键

* 触发机器人
  * 自定义热键
  * 延迟
    * 特定自定义延迟
    * 随机延迟（可设置范围）

* 漏洞利用
  * 无限生命
  * 无限弹药
  * 无扩散（效果不佳）
  * 穿墙（带自定义切换键选项）
  * 飞行作弊（带自定义切换键选项）
  * 速度作弊+速度滑块
  * 超级跳跃+跳跃高度滑块
  * 修改重力+重力滑块

## 待办事项
我未能自己完成菜单中的一些选项/功能，希望有人能帮忙：
* 静默瞄准（查看介绍中的静默瞄准部分了解原因）
* 使用敌人速度进行更精确的自动瞄准计算（我能获取敌人速度但实现后瞄准机器人非常卡顿）
* 无后坐力/扩散
* 查看FHitResult::GetActor()在UnrealEngine.cpp中的第一条评论：为什么我们要加3才能得到实际的序列号！？

如果你知道如何实现这些功能，请提交pull请求！

## 构建和编译
* 只能在VS22或更高版本编译，因为项目是用这个版本的Visual Studio创建的
* 在x64架构下构建和编译，因为Sword With Sauce是64位的
* 如果仍然无法编译，请在项目设置->配置属性->VC++目录->包含目录和库目录中设置正确的DirectX SDK路径

## UE简介
你需要采用特殊方法来黑客UE游戏。正如你在许多其他教程中会读到的，最重要的是找到**UWorld（又称GWorld）**，它包含了你可能需要的所有游戏信息。这通常被称为UWolrd，但实际上在UE源代码中类名是UWorld。你还会经常遇到**GNames和GObjects**，GNames基本上包含所有的类名，如果你想做比简单数值黑客更复杂的东西，你肯定需要**GNames**。在我的源代码中有一个函数mem::GetGName()展示了如何使用GNames，它位于UnrealEngine/Utils.cpp中。**GObjects**是一个类似GNames的数组，也可以类似地访问，不过我的项目中没有相关函数。据我所知，GObjects保存了游戏使用的所有类型的地址。

**WorldToScreen**函数以及许多其他使用的方法是通用的，意味着它们在所有虚幻引擎游戏中**相同或几乎相同**，所以你不需要完全理解某些东西，可以直接复制粘贴它们，无论这听起来多么廉价。实体列表或骨骼矩阵在什么结构体中不是游戏特定的，例如实体列表位于`UWorld->PersistentLevel->Actors`（只有偏移量有时会因虚幻引擎版本而异，但实体列表的路径相同）。我在`EntityClassObject->Mesh->CachedBoneArray`中找到了BoneMatrix。
另一个有用的路径是`UWorld->GameInstance->LocalPlayers[0]`，这**永远**是你的本地玩家的路径。

我未能在本项目中实现**静默瞄准**，因为这是一个离线游戏，意味着你需要钩住追踪线函数并修改其输出才能实现真正的静默瞄准，然而在所有现代在线游戏中这不是在玩家客户端完成的，意味着在在线游戏中实现静默瞄准的唯一方法是通过发送/伪造发送给服务器的视角。服务器会根据这些角度进行追踪线，意味着你会在服务器上击中敌人，尽管如果处理得当的话你本不应该击中。对于静默瞄准，你需要查看的两个函数是**ULocalPlayer::GetViewPoint()**和**APlayerController::GetPlayerViewPoint()**。这些函数返回关于本地玩家视角的信息，通过钩住它们你可以在大多数在线游戏中实现静默瞄准。不过不能保证对你的目标游戏有效。这里有一些[更多信息](https://github.com/RiceCum1/UE4-Silent-Aim?ref=https://githubhelp.com)。所以，回到离线游戏，你不能简单地钩住一个追踪线函数，首先因为你的游戏可能使用许多不同的虚幻引擎函数，所以你需要逆向游戏并找到ShootGun函数，在那里你会看到游戏使用什么追踪线函数。然后你需要钩住ShootGun函数并在程序中设置一个布尔值为true。在你的追踪线钩子中，你检查这个布尔值是否为true，如果是，你知道这个函数是被ShootGun函数调用的，从这里你可以返回一个错误的命中来模拟静默瞄准。这里有一个[相关教程](https://www.youtube.com/watch?v=paWftS5yhKU&t)。

## UE指南
我之前有Unity游戏黑客的经验，看起来绝对比虚幻引擎游戏黑客容易得多。所以如果你是初学者，我强烈建议从更简单的引擎开始。

无论如何，这是我的*旅程*：起初我甚至不知道虚幻引擎游戏是否需要**特殊方法**来黑客，正如我所学到的，它们确实需要。我阅读了guidedhacking.com上关于虚幻引擎游戏黑客的论坛，老实说，不是很好。我现在知道的大部分东西都是自学的，因为对于刚开始学习虚幻引擎的人来说真的没有很多好的教程。正如你在下面读到的，我使用了一个作弊引擎表来找到大多数结构偏移量。它更像是一个转储器，因为它自动转储最重要的偏移量如UWorld/GWorld，并给你一个脚本，你可以用它轻松逆向任何与游戏相关的类，就像你可以用cheat engine中的mono dissector一样。要启用这个选项，首先运行作弊表中的*Unreal Engine*脚本，然后打开*UE Structure Lookup*。我个人不是很确定要找到ShootGun函数，这就是为什么作弊没有这个选项。

我不得不下载几个不同版本的**作弊表**，直到找到一个适用于我的目标游戏的版本。另外应该很明显，但**不要在有反作弊的游戏中学习UE游戏黑客**。你是在为自己设置失败和脱发。如果你想要下载，我在Tools目录中附上了作弊表。这里是转储器的[论坛和下载](https://fearlessrevolution.com/viewtopic.php?f=23&t=14414)。一个关于作弊表的很棒的[教程](https://www.youtube.com/watch?v=iYPl9OOMack&t)。

你可以为你的目标游戏**生成一个SDK**，它将包含游戏使用的几乎所有结构/类。这非常有用且节省时间，但我个人没有这样做。说实话这看起来相当困难。你需要做的偏移量是**UWorld/GWorld, GNames, GObjects和UObject::ProcessEvent()**。

## 使用工具
* Visual Studio 2022 Preview
* [**Unreal Finder Tool**](https://github.com/CorrM/Unreal-Finder-Tool)
* [**Cheat Engine** + **UE4 Win64 Base Table**](https://fearlessrevolution.com/viewtopic.php?f=23&t=14414)
* **IDA Pro**（黑客UE游戏的必备工具）

不用说，你需要一个IDE来编译你的项目（黑客），所以我用了VS22。我个人没有太多使用**Unreal Finder Tool**，但你可以用它来**查找GNames, GObjects**以及生成SDK和转储GNames, GObjects（转储的意思是获取所有名称和对象，而不是GNames和GObjects的偏移量）。

**Cheat Engine**是经典，无论你黑客什么游戏。帮助我最多的是Cheat Engine的UE4 Win64 Base Table。它让你只需点击一下就能找到你的本地玩家和UWorld。另一个非常有用的东西是表中的UE Structurue Lookup脚本。它让你以非常好的格式查看游戏/虚幻引擎结构，就像Mono Dissector一样。我使用了工具的最新版本v6.9。

**注意：** *UE Structure Lookup*不会命名结构体内部的变量，即使它们不是垃圾。例如EntityClassObject->Mesh中的CachedBoneArray以及UWorld->PersistentLevel中的实体列表(actors)没有被转储器命名。

**IDA Pro**，也是游戏黑客中的经典。你将用它来查找偏移量+函数和逆向游戏函数。我个人用IDA Pro找任何虚幻引擎函数都没有问题。比如**UObject::ProcessEvent()**。找到虚幻引擎函数/偏移量的最好方法是下载你的游戏使用的虚幻引擎版本，然后用它构建一个简单的游戏。魔法在于.pdb，虚幻引擎可以生成它。如果你用链接的pdb打开生成的游戏exe，所有的虚幻引擎函数都会被命名。然后你可以简单地转到你想找的函数，复制操作码并在IDA中用签名扫描它们在另一个游戏中。正如你在这个项目中看到的，我也使用了模式/签名。这里有一个[特定教程](https://guidedhacking.com/threads/how-to-easily-make-signatures-for-unreal-engine-4-games.15529/)。

## 致谢
> 感谢：
* [omar](https://github.com/ocornut) 提供的优秀ImGui库
* [rdbo](https://github.com/rdbo/ImGui-DirectX-11-Kiero-Hook) 提供的项目模板
* [Guided Hacking](https://guidedhacking.com)