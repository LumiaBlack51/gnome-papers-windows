# Papers for Windows — 原地保存版

本仓库基于 GNOME Papers 和 [cosmicvi 的 Windows 移植](https://github.com/cosmicvi/papers-for-windows)，保留原项目历史和 GPL-2.0-or-later 许可。

## 安装

从本仓库 Releases 下载 `papers-*-installer-x64.exe`。默认安装到 `E:\Apps\Papers`，也可在安装向导中选择目录。安装不需要管理员权限，提供桌面、开始菜单快捷方式和卸载入口。

## 界面与启动

`51.beta.1-win-save.3` 将 Windows 版的界面字体统一放大为系统默认字体的约 1.5 倍，覆盖工具栏、菜单、侧栏、提示、Alert 及其他应用内对话框。启动首页、直接打开文件和新建窗口默认全屏，可按 F11 退出或重新进入全屏。

`51.beta.1-win-save.4` 同步放大关闭、最小化、最大化及工具栏的常用小图标，从 16px 调整为 24px，并增大对应按钮的点击区域。字体大小和启动全屏设置保持不变。

## 保存

- **Ctrl+S**：将批注、表单等修改保存回当前文件，不弹出另存对话框。
- **Ctrl+Shift+S**：另存一份副本；当前打开的文件路径保持不变。
- 关闭有修改的文档时，选择“保存”也会保存回原文件。
- 写入失败会显示错误并保留未保存状态。文件被其他程序锁定或无写入权限时，请解决问题后重试，或另存副本。

Windows PDF 后端从内存快照读取，避免占用原文件导致覆盖失败；内存用量会随 PDF 文件大小增加。保存仍使用临时文件，不再在替换失败后截断原文件。

## 批注编辑

点击右下角的“编辑文档”按钮后，可切换文字、画笔、高亮和橡皮工具。`51.beta.1-win-save.2` 补齐了 SVG 图片加载器依赖的运行库，修复打开编辑工具后报错的问题；光标图片加载失败时也可使用普通光标继续批注。同时修复编辑工具栏与页面输入层不同步的问题，切换工具后立即生效。退出编辑后恢复阅读，批注可用 Ctrl+S 保存。

## 从源码构建

在 MSYS2 UCRT64 安装 GCC、Meson、Ninja、Rust、GTK4、Libadwaita、Poppler、DjVuLibre、libarchive、libtiff、blueprint-compiler、gettext、appstream、NSIS 和 diffutils。所有 MinGW 包使用 `mingw-w64-ucrt-x86_64-` 前缀。

```powershell
.\build-aux\windows\build.ps1 -MsysRoot E:\BuildTools\msys64
```

脚本编译原生 x64 程序、运行 PDF 连续原地保存、失败保护、批注工具切换、缺少光标图片时的编辑及新增批注保存测试、打包运行库并生成安装程序。打包后再限制 PATH，只用随附运行库验证批注功能，避免构建环境掩盖漏打包的依赖。开发文档、Linux Nautilus 插件、打印预览器、缩略图服务和拼写检查在此构建中禁用。

本版本为社区构建，不是 GNOME 官方 Windows 发行版。安装程序未进行商业代码签名。
