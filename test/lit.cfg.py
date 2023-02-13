import os
import lit
from lit.llvm import llvm_config
from lit.llvm.subst import ToolSubst

config.name = "Custom Pass Test"
config.suffixes = ['.ll']
config.test_format = lit.formats.ShTest(True)
config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join("@CMAKE_CURRENT_BINARY_DIR@")

tools = ["opt", "FileCheck"]
llvm_config.add_tool_substitutions(tools, config.llvm_tools_dir)

config.substitutions.append(('%plugin_suffix', config.llvm_plugin_suffix))
config.substitutions.append(('%plugin_dir', config.llvm_plugin_dir))