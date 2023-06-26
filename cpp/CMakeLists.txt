# Copyright (C) 2020 IBM Corp.
# This program is Licensed under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#   http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License. See accompanying LICENSE file.

cmake_minimum_required(VERSION 3.10.2 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "-Werror -fopenmp -Wfatal-errors")

## Rely on the Environment variable to dictate which version of helib to use
set(HELIB_VERSION ${HELIB_CMAKE_LISTS_VERSON})

find_package(helib ${HELIB_VERSION} REQUIRED)
find_package(Boost 1.72.0 EXACT REQUIRED COMPONENTS filesystem)
find_package(OpenSSL REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS CXX)
include_directories(${HDF5_INCLUDE_DIR})

add_subdirectory(01_hebase_tutorial)
add_subdirectory(02_NeuralNetwork_FraudDetection)
add_subdirectory(arima)
add_subdirectory(BGV_world_country_db_lookup)
add_subdirectory(bitwise_tutorial)
add_subdirectory(circuit)
add_subdirectory(decision_tree)
add_subdirectory(er)
add_subdirectory(fhe_db)
add_subdirectory(game_of_life)
add_subdirectory(kmeans)
add_subdirectory(linear_regression)
add_subdirectory(logistic_regression)
add_subdirectory(multi_party_fhe)
add_subdirectory(psi_federated_learning)
add_subdirectory(query_database)
