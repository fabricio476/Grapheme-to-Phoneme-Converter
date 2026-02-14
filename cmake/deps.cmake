find_library(SONIC_LIB sonic)
find_path(SONIC_INC "sonic.h")
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

include(FetchContent)

if (CMAKE_USE_PTHREADS_INIT)
  set(HAVE_PTHREAD ON)
endif(CMAKE_USE_PTHREADS_INIT)
if (SONIC_LIB AND SONIC_INC)
  set(HAVE_LIBSONIC ON)
else()
  FetchContent_Declare(sonic-git
    GIT_REPOSITORY https://github.com/waywardgeek/sonic.git
    GIT_TAG fbf75c3d6d846bad3bb3d456cbc5d07d9fd8c104
  )
  FetchContent_MakeAvailable(sonic-git)
  FetchContent_GetProperties(sonic-git)
  add_library(sonic OBJECT ${sonic-git_SOURCE_DIR}/sonic.c)
  # enable PIC in order to embed libsonic.a in a .so
  set_property(TARGET sonic PROPERTY POSITION_INDEPENDENT_CODE ON)
  target_include_directories(sonic PUBLIC ${sonic-git_SOURCE_DIR})
  set(HAVE_LIBSONIC ON)
  set(SONIC_LIB sonic)
  set(SONIC_INC ${sonic-git_SOURCE_DIR})
endif()
