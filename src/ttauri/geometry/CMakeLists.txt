
target_sources(ttauri PRIVATE
    matrix.hpp
    identity.hpp
    translate.hpp
    rotate.hpp
    scale.hpp
    transform.hpp
    point.hpp
    vector.hpp
    extent.hpp
)

target_sources(ttauri_tests PRIVATE
    matrix_tests.cpp
    identity_tests.cpp
    translate_tests.cpp
    scale_tests.cpp
    transform_tests.cpp
    point_tests.cpp
    vector_tests.cpp
)
