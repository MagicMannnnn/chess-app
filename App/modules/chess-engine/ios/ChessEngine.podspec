Pod::Spec.new do |s|
  s.name           = 'ChessEngine'
  s.version        = '1.0.0'
  s.summary        = 'Native C++ chess engine module for React Native'
  s.description    = 'A high-performance chess engine written in C++ and exposed as a native module for React Native applications'
  s.license        = { :type => 'MIT' }
  s.author         = 'Chess App'
  s.homepage       = 'https://github.com/MagicMannnnn/chess-app'
  s.platform       = :ios, '13.4'
  s.swift_version  = '5.4'
  s.source         = { :git => 'https://github.com/MagicMannnnn/chess-app.git', :tag => "v#{s.version}" }
  s.static_framework = true

  s.dependency 'ExpoModulesCore'

  # iOS wrapper files and C++ engine source files (symlinked into ios directory)
  s.source_files = "*.{h,m,mm,swift,cpp}", "v1/**/*.{h,cpp}"
  
  # Only expose the Objective-C++ wrapper header, not the C++ headers
  s.public_header_files = "ChessEngineWrapper.h"
  
  # C++ headers should be private (for internal use only)
  s.private_header_files = "ChessEngine.h", "Board.h", "Move.h", "Types.h", "v1/Evaluation.h", "v1/Search.h"
  
  # Compiler settings - make sure to compile as C++
  s.compiler_flags = '-std=c++17'
  
  # Header search paths and C++ settings
  s.pod_target_xcconfig = {
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17',
    'CLANG_CXX_LIBRARY' => 'libc++',
    'HEADER_SEARCH_PATHS' => '$(inherited) $(PODS_TARGET_SRCROOT)/../cpp/engine',
    'USER_HEADER_SEARCH_PATHS' => '$(inherited) $(PODS_TARGET_SRCROOT)/../cpp/engine',
    'GCC_PREPROCESSOR_DEFINITIONS' => '$(inherited)',
    'OTHER_LDFLAGS' => '$(inherited) -lc++',
    'OTHER_CPLUSPLUSFLAGS' => '$(OTHER_CFLAGS) -std=c++17'
  }
end
