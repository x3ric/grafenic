#!/bin/bash -i

CC="clang -w -g -Wextra"
FREETYPE="$(pkg-config --cflags freetype2)"
CFLAGS="-I./deps -I./src"
LDFLAGS="-lglfw -lGL -lGLEW -lm -lfreetype"
TARGET="grafenic"

need() {
    package="$1"
    if pacman -Q "$package" &> /dev/null; then
        return 0
    fi
    if command -v yay &> /dev/null; then
        echo "Installing $package using yay."
        yay --needed -S "$package"
        return $?
    fi
    if command -v pacman &> /dev/null; then
        echo "Installing $package using pacman."
        sudo pacman --needed -Sy "$package"
        return $?
    fi
    echo "Cannot install $package. Please install it manually using your distro's package manager."
    return 1
}

need clang
need fzf

clean() {
    echo -e "rm -rf ./$TARGET ./build/"
    rm -rf ./$TARGET ./build/
}

install() {
    sudo cp -r ./deps/* /usr/local/include/
    sudo mkdir -p /usr/local/include/grafenic
    sudo cp ./build/libgrafenic.so /usr/local/lib/
    sudo cp ./src/window.h /usr/local/include/grafenic/
    sudo ldconfig
}

uninstall() {
    sudo rm -f /usr/local/lib/libgrafenic.so
    sudo rm -rf /usr/local/include/grafenic
}

fzf-splitted () {
    if [[ -n "$TMUX" ]]; then
        fzf --color=16 --reverse --ansi "$@"
    else
        fzf-tmux --color=16 -x --height ${FZF_TMUX_HEIGHT:-40%} --reverse --cycle --ansi "$@"
    fi
}

compile-library() {
    echo "$(find ./src -type f -exec md5sum {} + | sort | md5sum | cut -d' ' -f1)" > ./build/src.hash
    echo -e "$CC $CFLAGS \"\$(pkg-config --cflags freetype2)\" -fPIC -c src/window.c -o ./build/window.o"
    $CC $CFLAGS $FREETYPE -fPIC -c src/window.c -o ./build/window.o
    echo -e "$CC $CFLAGS  \"\$(pkg-config --cflags freetype2)\" -shared -o ./build/libgrafenic.so ./build/window.o $LDFLAGS"
    $CC $CFLAGS  $FREETYPE -shared -o ./build/libgrafenic.so ./build/window.o $LDFLAGS
}

build() {
    echo -e "rm -rf ./$TARGET"
    rm -rf ./$TARGET
    if [ -f "./build/src.hash" ]; then
        if [ "$(find ./src -type f -exec md5sum {} + | sort | md5sum | cut -d' ' -f1)" != "$(cat ./build/src.hash)" ]; then
            echo -e "rm -rf ./build/*"
            rm -rf ./build/
            uninstall
        fi
    fi
    if [ -z "$1" ]; then
        SOURCES="./projects/$(ls ./projects | grep -v '^modules$' | sed 's/\.c$//' | fzf-splitted).c"
    else
        SOURCES="./projects/$1.c"
    fi
    mkdir -p ./build/
    if [ ! -f "/usr/local/lib/libgrafenic.so" ]; then
        compile-library
        install
    fi
    CFLAGS="-I./deps -I./projects"
    export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
    if [ "$SOURCES" == "./projects/editor.c" ]; then
        need tree-sitter
        need tree-sitter-c
        echo -e "$CC $CFLAGS \$(pkg-config --cflags freetype2) $SOURCES -o $TARGET \$(pkg-config --libs tree-sitter) -ldl -lgrafenic $LDFLAGS"
        $CC $CFLAGS $FREETYPE $SOURCES -o $TARGET $(pkg-config --libs tree-sitter) -lgrafenic $LDFLAGS
    else
        echo -e "$CC $CFLAGS \$(pkg-config --cflags freetype2) $SOURCES -o $TARGET -ldl -lgrafenic $LDFLAGS"
        $CC $CFLAGS $FREETYPE $SOURCES -o $TARGET -lgrafenic $LDFLAGS
    fi
}

run() {
    build $1
    echo "./$TARGET $2"
    "./$TARGET" $2
}

debug() {
    build $1
    gdb -q $TARGET --eval-command=run
}

case $1 in
    run)
        run $2 ${@:3}
        ;;
    build)
        build $2
        ;;
    debug)
        debug $2
        ;;
    clean)
        clean
        ;;
    install)
        install $2
        ;;
    uninstall)
        uninstall
        ;;
    *)
        echo "Usage: $0 {install|uninstall|build|run|debug|clean}"
        exit 1
        ;;
esac
