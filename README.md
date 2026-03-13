# skill-gov

管理 claude 技能

## 功能

- `--list` 列出技能启用/禁用状态
- `enable <patterns...>` 批量启用（支持 `*` 和 `?`）
- `disable <patterns...>` 批量禁用（支持 `*` 和 `?`）
- `enableall` 一键启用全部技能
- `disableall` 一键禁用全部技能（便于重新挑选）
- `--dry-run` 预览变更
- 批量移动失败时自动回滚

## 编译

```sh
make all
# 或
make debug
make sanitize
```

产物：

- `build/release/skill-gov`
- `build/debug/skill-gov`
- `build/sanitize/skill-gov`

## 安装

```sh
make install
```

默认安装到 `~/.local/bin/skill-gov`。

## 用法

```sh
skill-gov --list
skill-gov disableall --dry-run
skill-gov enableall --dry-run
skill-gov disable 'actix-*'
skill-gov enable 'flutter-*' --dry-run
```

