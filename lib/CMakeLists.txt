set(
    LLVM_CUSTOM_PLUGINS
    FuncInfoAnalysis
    FuncInfoPrinter
    Mem2Reg
)

set(
    FuncInfoAnalysisSource
    FuncInfoAnalysis.cpp
)
set(
    FuncInfoPrinterSource
    FuncInfoPrinter.cpp
)
set(
  Mem2RegSource
  Mem2Reg.cpp
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