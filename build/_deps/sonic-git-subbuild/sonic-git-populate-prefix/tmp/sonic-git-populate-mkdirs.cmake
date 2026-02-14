# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/mnt/dados/Projeto g2p/meu-g2p/build/_deps/sonic-git-src")
  file(MAKE_DIRECTORY "/mnt/dados/Projeto g2p/meu-g2p/build/_deps/sonic-git-src")
endif()
file(MAKE_DIRECTORY
  "/mnt/dados/Projeto g2p/meu-g2p/build/_deps/sonic-git-build"
  "/mnt/dados/Projeto g2p/meu-g2p/build/_deps/sonic-git-subbuild/sonic-git-populate-prefix"
  "/mnt/dados/Projeto g2p/meu-g2p/build/_deps/sonic-git-subbuild/sonic-git-populate-prefix/tmp"
  "/mnt/dados/Projeto g2p/meu-g2p/build/_deps/sonic-git-subbuild/sonic-git-populate-prefix/src/sonic-git-populate-stamp"
  "/mnt/dados/Projeto g2p/meu-g2p/build/_deps/sonic-git-subbuild/sonic-git-populate-prefix/src"
  "/mnt/dados/Projeto g2p/meu-g2p/build/_deps/sonic-git-subbuild/sonic-git-populate-prefix/src/sonic-git-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/mnt/dados/Projeto g2p/meu-g2p/build/_deps/sonic-git-subbuild/sonic-git-populate-prefix/src/sonic-git-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/mnt/dados/Projeto g2p/meu-g2p/build/_deps/sonic-git-subbuild/sonic-git-populate-prefix/src/sonic-git-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
