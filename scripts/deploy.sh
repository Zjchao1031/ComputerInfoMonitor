#!/bin/sh
# ============================================================================
# ComputerInfoMonitor 构建与部署脚本（i.MX6ULL / Buildroot 2025.02.17）。
#
# 本脚本在 Ubuntu 24.04 虚拟机中执行，一次完成：
#   1. 清理并交叉构建 build/board/release/computer-info-monitor；
#   2. 将可执行程序、monitor.ini 和 BusyBox init 脚本部署到开发板挂载的
#      根文件系统（开发期默认使用 /home/zjc/linux/nfs/rootfs）；
#   3. 默认保留板端配置与目录内容；使用 -o 时清空应用目录后完整部署。
#
# 板端启动流程：
#   /etc/init.d/S99computer-info-monitor
#       -> /opt/computer-info-monitor/computer-info-monitor
#
# 用法：
#   sh scripts/deploy.sh [选项]
#
# 选项：
#   --rootfs DIR    板端根文件系统挂载目录，默认 $CIM_ROOTFS_DIR，再默认
#                   /home/zjc/linux/nfs/rootfs
#   --no-build      跳过清理与构建，直接部署现有构建产物
#   -o              清空 /opt/computer-info-monitor 后完整部署
#   -h, --help      显示帮助
#
# 可覆盖的环境变量：
#   CIM_ROOTFS_DIR          板端根文件系统挂载目录
#   CIM_BUILD_DIR           构建目录，默认 <项目>/build/board/release
#   CIM_BUILDROOT_HOST_DIR  Buildroot output/host 目录
#   CIM_SYSROOT             目标 sysroot，默认 Buildroot 对应 sysroot
#   QT_ARM_PREFIX           目标 Qt 安装前缀
#   QT_HOST_PATH            主机侧 Qt 前缀（需要交叉编译主机工具时设置）
#   CIM_BUILD_JOBS          Ninja 并行任务数，默认交给 CMake/Ninja 决定
#   CIM_CMAKE               CMake 命令或绝对路径
#   CIM_NINJA               Ninja 可执行文件路径
#
# 注意：默认部署会保留板端现有 monitor.ini 和其他文件，仅在配置缺失时安装
# 仓库模板；使用 -o 会清空应用目录，并用仓库模板覆盖配置。
# 部署后如程序正在运行，请在开发板上执行
# /etc/init.d/S99computer-info-monitor restart（或重启开发板）。
# ============================================================================

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

CIM_BUILDROOT_HOST_DIR=${CIM_BUILDROOT_HOST_DIR:-/home/zjc/linux/buildroot/buildroot-2025.02.17/output/host}
CIM_BUILD_DIR=${CIM_BUILD_DIR:-"$PROJECT_DIR/build/board/release"}
CIM_BUILD_JOBS=${CIM_BUILD_JOBS:-}
CIM_CMAKE=${CIM_CMAKE:-cmake}
CIM_NINJA=${CIM_NINJA:-"$CIM_BUILDROOT_HOST_DIR/bin/ninja"}
CIM_ROOTFS_DIR=${CIM_ROOTFS_DIR:-/home/zjc/linux/nfs/rootfs}
CIM_SYSROOT=${CIM_SYSROOT:-${IMX6ULL_SYSROOT:-"$CIM_BUILDROOT_HOST_DIR/arm-buildroot-linux-gnueabihf/sysroot"}}
QT_ARM_PREFIX=${QT_ARM_PREFIX:-"$CIM_SYSROOT/usr"}
QT_HOST_PATH=${QT_HOST_PATH:-}

TOOLCHAIN_FILE="$PROJECT_DIR/cmake/linux-arm-imx6ull.cmake"
CONFIG_TEMPLATE="$PROJECT_DIR/monitor.ini"
INIT_TEMPLATE="$SCRIPT_DIR/S99computer-info-monitor"
BUILD_BINARY="$CIM_BUILD_DIR/computer-info-monitor"

ROOTFS_APP_SUBDIR=opt/computer-info-monitor
ROOTFS_INIT_SUBDIR=etc/init.d/S99computer-info-monitor

rootfs_dir=$CIM_ROOTFS_DIR
do_build=1
clear_deployment=0

usage() {
    cat <<'EOF'
用法：sh scripts/deploy.sh [选项]

选项：
  --rootfs DIR    板端根文件系统挂载目录，默认 $CIM_ROOTFS_DIR 或
                  /home/zjc/linux/nfs/rootfs
  --no-build      跳过清理与构建，直接部署现有构建产物
  -o              清空 /opt/computer-info-monitor 后完整部署
  -h, --help      显示帮助

部署语义：默认可执行程序与 /etc/init.d/S99computer-info-monitor 会被替换，
板端现有 monitor.ini 和其他文件保持不变；monitor.ini 缺失时安装仓库模板。
使用 -o 会清空 /opt/computer-info-monitor，再重新部署全部文件。
EOF
}

die() {
    printf 'ComputerInfoMonitor 部署失败：%s\n' "$*" >&2
    exit 1
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --rootfs)
            [ "$#" -ge 2 ] || die "--rootfs 缺少目录参数"
            rootfs_dir=$2
            shift 2
            ;;
        --no-build)
            do_build=0
            shift
            ;;
        -o)
            clear_deployment=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        -*)
            usage >&2
            die "未知选项：$1"
            ;;
        *)
            usage >&2
            die "本脚本不接受位置参数：$1"
            ;;
    esac
done

[ -n "$rootfs_dir" ] || die "板端根文件系统挂载目录为空"

check_build_tools() {
    [ -d "$CIM_BUILDROOT_HOST_DIR" ] ||
        die "Buildroot host 目录不存在：$CIM_BUILDROOT_HOST_DIR"
    if [ ! -x "$CIM_NINJA" ] && ! command -v "$CIM_NINJA" >/dev/null 2>&1; then
        die "Ninja 不存在或不可执行：$CIM_NINJA"
    fi
    if [ ! -x "$CIM_CMAKE" ] && ! command -v "$CIM_CMAKE" >/dev/null 2>&1; then
        die "CMake 不存在或不可执行：$CIM_CMAKE"
    fi
    [ -f "$TOOLCHAIN_FILE" ] ||
        die "交叉编译工具链文件不存在：$TOOLCHAIN_FILE"
    [ -f "$QT_ARM_PREFIX/lib/cmake/Qt6/Qt6Config.cmake" ] ||
        die "未找到目标 Qt 6 包配置：$QT_ARM_PREFIX/lib/cmake/Qt6/Qt6Config.cmake（可用 QT_ARM_PREFIX 覆盖）"
    command -v arm-linux-gnueabihf-gcc-13 >/dev/null 2>&1 ||
        die "未找到交叉编译器：arm-linux-gnueabihf-gcc-13"
    command -v arm-linux-gnueabihf-g++-13 >/dev/null 2>&1 ||
        die "未找到交叉编译器：arm-linux-gnueabihf-g++-13"
}

normalize_build_dir() {
    command -v realpath >/dev/null 2>&1 ||
        die "缺少 realpath 命令，无法安全校验构建目录"

    project_real=$(realpath -e -- "$PROJECT_DIR") ||
        die "无法解析项目目录：$PROJECT_DIR"
    build_real=$(realpath -m -- "$CIM_BUILD_DIR") ||
        die "无法解析构建目录：$CIM_BUILD_DIR"
    CIM_BUILD_DIR=$build_real
    BUILD_BINARY="$CIM_BUILD_DIR/computer-info-monitor"

    case "$CIM_BUILD_DIR" in
        ''|/|"$project_real"|"$project_real/build")
            die "拒绝清理不安全的构建目录：$CIM_BUILD_DIR"
            ;;
    esac

    case "$CIM_BUILD_DIR" in
        "$project_real"/build/*)
            ;;
        *)
            die "构建目录必须位于 $project_real/build 内：$CIM_BUILD_DIR"
            ;;
    esac
}

clean_build_dir() {
    normalize_build_dir

    if [ -e "$CIM_BUILD_DIR" ] || [ -L "$CIM_BUILD_DIR" ]; then
        printf '清理构建目录：%s\n' "$CIM_BUILD_DIR"
        rm -rf -- "$CIM_BUILD_DIR"
    fi
}

build_application() {
    check_build_tools
    clean_build_dir

    set -- \
        -S "$PROJECT_DIR" \
        -B "$CIM_BUILD_DIR" \
        -G Ninja \
        -DCMAKE_MAKE_PROGRAM="$CIM_NINJA" \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$QT_ARM_PREFIX"

    if [ -n "$CIM_SYSROOT" ]; then
        set -- "$@" "-DIMX6ULL_SYSROOT=$CIM_SYSROOT"
    fi

    if [ -n "$QT_HOST_PATH" ]; then
        set -- "$@" "-DQT_HOST_PATH=$QT_HOST_PATH"
    elif [ -f "$CIM_BUILDROOT_HOST_DIR/lib/cmake/Qt6/Qt6Config.cmake" ]; then
        set -- "$@" "-DQT_HOST_PATH=$CIM_BUILDROOT_HOST_DIR"
    fi

    printf '配置交叉构建：%s\n' "$CIM_BUILD_DIR"
    "$CIM_CMAKE" "$@"

    if [ -n "$CIM_BUILD_JOBS" ]; then
        "$CIM_CMAKE" --build "$CIM_BUILD_DIR" --parallel "$CIM_BUILD_JOBS"
    else
        "$CIM_CMAKE" --build "$CIM_BUILD_DIR" --parallel
    fi

    [ -f "$BUILD_BINARY" ] || die "构建完成但未找到产物：$BUILD_BINARY"
    printf '构建完成：%s\n' "$BUILD_BINARY"
}

check_rootfs() {
    [ -d "$rootfs_dir" ] ||
        die "板端根文件系统挂载目录不存在：$rootfs_dir（可用 --rootfs 或 CIM_ROOTFS_DIR 覆盖）"
    [ -d "$rootfs_dir/etc/init.d" ] ||
        die "$rootfs_dir 不像是板端根文件系统：缺少 etc/init.d"
    [ -d "$rootfs_dir/opt" ] ||
        die "$rootfs_dir 不像是板端根文件系统：缺少 opt"
    [ -w "$rootfs_dir/opt" ] ||
        die "无写权限：$rootfs_dir/opt（rootfs 内文件属 root，请用 sudo 执行本脚本）"
    [ -w "$rootfs_dir/etc/init.d" ] ||
        die "无写权限：$rootfs_dir/etc/init.d（rootfs 内文件属 root，请用 sudo 执行本脚本）"

    command -v realpath >/dev/null 2>&1 ||
        die "缺少 realpath 命令，无法安全校验板端目录"
    rootfs_dir=$(realpath -e -- "$rootfs_dir") ||
        die "无法解析板端根文件系统目录：$rootfs_dir"
    [ "$rootfs_dir" != "/" ] ||
        die "拒绝直接部署到当前系统根目录 /"

    app_dir=$(realpath -m -- "$rootfs_dir/$ROOTFS_APP_SUBDIR") ||
        die "无法解析板端应用目录"
    init_script=$(realpath -m -- "$rootfs_dir/$ROOTFS_INIT_SUBDIR") ||
        die "无法解析板端启动脚本路径"

    case "$app_dir" in
        "$rootfs_dir"/opt/computer-info-monitor)
            ;;
        *)
            die "板端应用目录越出根文件系统：$app_dir"
            ;;
    esac

    case "$init_script" in
        "$rootfs_dir"/etc/init.d/S99computer-info-monitor)
            ;;
        *)
            die "板端启动脚本越出根文件系统：$init_script"
            ;;
    esac
}

install_file() {
    source_file=$1
    target_file=$2
    target_mode=$3
    temporary_file="$target_file.new"

    rm -f -- "$temporary_file"
    cp -- "$source_file" "$temporary_file" ||
        die "拷贝失败：$source_file -> $target_file"
    chmod "$target_mode" "$temporary_file" ||
        die "设置权限失败：$temporary_file"
    mv -f -- "$temporary_file" "$target_file" ||
        die "替换失败：$target_file"
}

clear_app_dir() {
    case "$app_dir" in
        ''|/|"$rootfs_dir"|"$rootfs_dir/")
            die "拒绝清理不安全的板端应用目录：$app_dir"
            ;;
    esac

    if [ -d "$app_dir" ]; then
        rm -rf -- "$app_dir" || die "清空目录失败：$app_dir"
    fi
}

install_to_rootfs() {
    printf '部署到板端根文件系统：%s\n' "$rootfs_dir"

    mkdir -p "$app_dir" || die "无法创建目录：$app_dir"
    if [ "$clear_deployment" -eq 1 ]; then
        clear_app_dir
        mkdir -p "$app_dir" || die "无法创建目录：$app_dir"
        printf '已清空部署目录：%s\n' "$app_dir"
    fi

    install_file "$BUILD_BINARY" "$app_dir/computer-info-monitor" 0755
    printf '已安装程序：%s\n' "$app_dir/computer-info-monitor"

    if [ -f "$app_dir/monitor.ini" ]; then
        printf '保留板端配置：%s\n' "$app_dir/monitor.ini"
    elif [ -e "$app_dir/monitor.ini" ]; then
        die "板端配置路径不是普通文件：$app_dir/monitor.ini"
    else
        install_file "$CONFIG_TEMPLATE" "$app_dir/monitor.ini" 0644
        printf '已安装配置模板：%s\n' "$app_dir/monitor.ini"
    fi

    install_file "$INIT_TEMPLATE" "$init_script" 0755
    printf '已安装自启动脚本：%s\n' "$init_script"
}

[ "$(uname -s)" = "Linux" ] ||
    die "本脚本需在 Ubuntu 虚拟机内执行（当前系统不是 Linux）"

[ -f "$CONFIG_TEMPLATE" ] || die "未找到配置模板：$CONFIG_TEMPLATE"
[ -f "$INIT_TEMPLATE" ] || die "未找到 init 脚本：$INIT_TEMPLATE"

if [ "$do_build" -eq 1 ]; then
    build_application
else
    normalize_build_dir
    printf '按 --no-build 跳过清理与构建\n'
    [ -f "$BUILD_BINARY" ] ||
        die "未找到构建产物：$BUILD_BINARY（去掉 --no-build 重新构建）"
fi

check_rootfs
install_to_rootfs

printf '部署完成：%s\n' "$app_dir"
printf '板端启动流程：%s 启动 %s\n' "$init_script" "$app_dir/computer-info-monitor"
printf '程序运行中时，请在开发板上执行 %s restart（或重启开发板）加载新版本。\n' "$init_script"
