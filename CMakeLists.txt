cmake_minimum_required(VERSION 3.16)
project(avp_jaguar_c_decomp C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(AVP_STRICT "Compile readable C with strict warnings" ON)

add_library(avp_c STATIC
  src/game/main_game.c
  src/game/mazescrn.c
  src/game/maze.c
  src/game/runtime.c
  src/game/files.c
  src/game/sprites.c
  src/game/music.c
  src/game/message.c
  src/game/objects.c
  src/game/font.c
  src/game/computer.c
  src/game/mjp.c
  src/platform/jaguar/blitter.c
  src/platform/jaguar/joypad.c
  src/platform/jaguar/jaguar.c
  src/game/joyed.c
  src/game/player.c
  src/game/gpu_overlays.c
  src/game/collision.c
  src/game/hud_message.c
  src/game/hud.c
  src/game/hud_score.c
  src/game/weapons.c
  src/game/collectables.c
  src/game/amp.c
  src/game/eeprom.c
  src/game/levels.c
  src/game/doors.c
)
target_include_directories(avp_c PUBLIC include)

# ROM bootstrap support is ordinary 68000 C but is built separately because it
# has a historical allocator ABI and is not part of the RAM-resident game lib.
add_library(avp_unzip_alloc STATIC src/unzip/alloc_reconstructed.c)
add_library(avp_unzip_inflate STATIC src/unzip/inflate_reconstructed.c)
target_include_directories(avp_unzip_alloc PUBLIC include)
target_include_directories(avp_unzip_inflate PUBLIC include)

if(AVP_STRICT)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(avp_c PRIVATE -Wall -Wextra -Werror)
    target_compile_options(avp_unzip_inflate PRIVATE -Wall -Wextra -Werror)
    # This one intentionally models the 1994 K&R malloc/free ABI.
    target_compile_options(avp_unzip_alloc PRIVATE -Wall -Wextra
      -Wno-builtin-declaration-mismatch -Wno-free-nonheap-object)
  endif()
endif()
