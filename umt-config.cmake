find_package(fmt REQUIRED)
find_package(Threads REQUIRED)

set(umt_LIBS fmt::fmt Threads::Threads)
set(umt_INCLUDE_DIR include)