# 10 GitHub 工作流教程

本文按 Windows PowerShell 编写，目标是让第一次使用 GitHub 的开发者理解每一步，而不是只会上传 ZIP。

## 10.1 Git 的三个区域

```text
工作区        git add        暂存区        git commit        本地仓库
修改中的文件  ----------->  准备提交的文件  ------------->  历史版本
                                                               |
                                                               | git push
                                                               v
                                                            GitHub
```

- `git add`：选择这次提交哪些修改；
- `git commit`：在本地生成一个版本；
- `git push`：把本地版本上传到 GitHub；
- `git pull`：拉取远端的新版本。

## 10.2 第一次安装和身份配置

安装 Git for Windows 后打开 PowerShell：

```powershell
git --version
git config --global user.name "Hui He"
git config --global user.email "1845915574@qq.com"
git config --global init.defaultBranch main
```

检查：

```powershell
git config --global --list
```

## 10.3 在 GitHub 创建空仓库

仓库名：

```text
DrivePilot-Cockpit
```

建议设置：

- Visibility：Public；
- 不勾选 Add a README；
- 不勾选 `.gitignore`；
- 不选择 License。

原因：本地交付包已经包含这些文件，远端必须保持空仓库，第一次推送最简单。

## 10.4 首次上传

将 `DrivePilot-Cockpit-GitHub-Ready.zip` 解压到一个稳定目录，例如：

```text
D:\GitHub\DrivePilot-Cockpit
```

不要直接在压缩包、下载缓存或临时目录中操作。

```powershell
cd D:\GitHub\DrivePilot-Cockpit
python scripts/pre_push_check.py

git init
git status
git add .
git status
git commit -m "feat: release DrivePilot Cockpit v1.0.0"
git remote add origin https://github.com/WaitsKid/DrivePilot-Cockpit.git
git branch -M main
git push -u origin main
```

第一次 `push` 通常会弹出浏览器登录窗口。完成授权后 Git Credential Manager 会保存凭据，不需要把 GitHub 密码写进命令。

## 10.5 上传后检查

打开仓库网页并确认：

- README 正常渲染；
- `config.json` 不存在；
- `.env` 不存在；
- 没有 `.venv`、`cmake-build-*`、`__pycache__`；
- Mermaid 图可以显示；
- 三个子项目目录完整；
- Actions 中 Python 测试通过。

## 10.6 日常修改

```powershell
git status
git diff
git add 修改的文件
git commit -m "fix: correct DMS voice toggle behavior"
git push
```

不要每次都机械使用 `git add .`。熟练后应只暂存本次相关文件：

```powershell
git add hmi-client/Interface/DmsController.cpp
git add docs/07-user-guide.md
```

## 10.7 推荐提交格式

```text
feat: 新功能
fix: 修复缺陷
docs: 文档
refactor: 重构但不改变功能
test: 测试
build: CMake/依赖
chore: 杂项
```

示例：

```text
feat: add tool-calling agent backend
fix: stop DMS camera when reminder is disabled
docs: add deployment and privacy guide
test: cover fatigue state recovery
```

## 10.8 分支工作流

小型个人项目也建议学习分支：

```powershell
git switch -c feature/rag-manual
# 修改代码
git add .
git commit -m "feat: add vehicle manual retrieval prototype"
git push -u origin feature/rag-manual
```

然后在 GitHub 创建 Pull Request，检查差异后合并到 `main`。

## 10.9 撤销操作

### 取消工作区修改

```powershell
git restore 文件路径
```

### 取消暂存但保留修改

```powershell
git restore --staged 文件路径
```

### 修改最近一次提交说明

```powershell
git commit --amend
```

### 查看历史

```powershell
git log --oneline --graph --decorate --all
```

不要在不了解后果时使用：

```text
git reset --hard
git push --force
```

## 10.10 Key 误传后的处理

1. 立即在讯飞、高德或 Kimi 控制台禁用/轮换 Key；
2. 不要只提交一个“删除 Key”的新版本；
3. 使用 `git filter-repo` 或 BFG 清理历史；
4. 强制推送清理后的历史；
5. 再次运行仓库秘密扫描。

## 10.11 Release

代码稳定后，在 GitHub：

```text
Releases → Draft a new release
Tag: v1.0.0
Title: DrivePilot Cockpit v1.0.0
```

Release 不要上传：

- 数据集；
- `.env`、`config.json`；
- 私人媒体；
- 未确认授权的模型权重；
- Windows 构建目录。

可以上传：

- 经过部署工具打包的演示程序；
- 使用说明；
- 已确认许可的模型；
- 校验和。
