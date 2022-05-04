CFLAGS="-Wall -pedantic -O2"
TOOLCHAIN=${TOOLCHAIN:-}
TARGET=${TARGET:-mc}

${TOOLCHAIN}g++ common/minecraft.cpp -c -o pc/minecraft.o ${CFLAGS}
${TOOLCHAIN}g++ pc/main.cc -o ${TARGET} pc/minecraft.o -Icommon ${CFLAGS}
${TOOLCHAIN}strip -s ${TARGET}

