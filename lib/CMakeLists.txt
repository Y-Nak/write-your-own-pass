set(
    LLVM_CUSTOM_PLUGINS
    FuncInfoAnalysis
)
set(
    FuncInfoAnalysisSource
    FuncInfoAnalysis.cpp
)

foreach(plugin ${LLVM_CUSTOM_PLUGINS})
  add_library(${plugin} SHARED ${${plugin}Source})

  target_include_directories(
      ${plugin}
      PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/../include"
  )

  target_link_libraries(
    ${plugin}
    "$<$<PLATFORM_ID:Darwin>:-undefined dynamic_lookup>"
  )
endforeach()