# Papers for Windows — 原地保存版

本仓库基于 GNOME Papers 和 [cosmicvi 的 Windows 移植](https://github.com/cosmicvi/papers-for-windows)，保留原项目历史和 GPL-2.0-or-later 许可。

## 安装

从本仓库 Releases 下载 `papers-*-installer-x64.exe`。默认安装到 `E:\Apps\Papers`，也可在安装向导中选择目录。安装不需要管理员权限，提供桌面、开始菜单快捷方式和卸载入口。

## 保存

- **Ctrl+S**：将批注、表单等修改保存回当前文件，不弹出另存对话框。
- **Ctrl+Shift+S**：另存一份副本；当前打开的文件路径保持不变。
- 关闭有修改的文档时，选择“保存”也会保存回原文件。
- 写入失败会显示错误并保留未保存状态。文件被其他程序锁定或无写入权限时，请解决问题后重试，或另存副本。

Windows PDF 后端从内存快照读取，避免占用原文件导致覆盖失败；内存用量会随 PDF 文件大小增加。保存仍使用临时文件，不再在替换失败后截断原文件。

## 从源码构建

在 MSYS2 UCRT64 安装 GCC、Meson、Ninja、Rust、GTK4、Libadwaita、Poppler、DjVuLibre、libarchive、libtiff、blueprint-compiler、gettext、appstream、NSIS 和 diffutils。所有 MinGW 包使用 `mingw-w64-ucrt-x86_64-` 前缀。

```powershell
.\build-aux\windows\build.ps1 -MsysRoot E:\BuildTools\msys64
```

脚本编译原生 x64 程序、运行 PDF 连续原地保存及失败保护测试、打包运行库并生成安装程序。开发文档、Linux Nautilus 插件、打印预览器、缩略图服务和拼写检查在此构建中禁用。

本版本为社区构建，不是 GNOME 官方 Windows 发行版。安装程序未进行商业代码签名。
