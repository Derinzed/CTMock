# CTMock (Compile Time Mock)
A fast, lightweight, compile-time safe C++ mocking framework — designed for projects where minimal runtime overhead and strong type guarantees matter.

Overview
CompileTimeMocking (CTM) is a C++ header-only mocking library designed to make unit testing large and complex codebases simpler and more reliable.
Instead of relying on runtime polymorphism and traditional dependency injection, CTM uses compile-time templates and policies to generate mocks that are strongly typed and efficient.

This approach allows developers to:
- Mock large systems without needing intrusive class refactoring.
- Ensure type safety at compile time.
- Modify behavior at runtime while keeping strong type guarantees.

# Key Features
- Compile-time generation of mocks with minimal runtime dispatch.
- Header-only, no build system changes required.
- Mock static methods, free functions, and classes easily.
- Flexible runtime return value management through matchers.
- No dependency injection necessary for basic usage.
- Safe for legacy codebases without heavy modification.

# Why CTMock?
Mocking frameworks like Google Mock (gMock) and others excel in many cases — particularly when full dynamic behavior or advanced runtime configuration is needed.

CTM provides an alternative philosophy:

- Focused on compile-time safety.
- Ideal for performance-sensitive, resource-constrained, or legacy projects.
- Designed for scenarios where dependency injection may not be feasible.
- Built to mock existing systems without rewriting for abstract interfaces.

Rather than replacing traditional frameworks, CTM complements them:
Use the right tool for the right kind of project.

# Quick Examples
Define a mock class:

```
CTM_CLASS(MockFilesystem)
    COMPILE_MOCK(exists, bool, const std::string)
    COMPILE_MOCK(is_directory, bool, const std::string)
    COMPILE_MOCK(all_directories, std::vector<std::string>, const std::string)
CTM_CLASS_END
Set up expectations:


using namespace MetalFury::Mocks; // Or your namespace

CTM_MOCK.setupForMocking(MockFilesystem::exists, true);
CTM_MOCK.setupForMocking(MockFilesystem::is_directory, true);

std::vector<std::string> fakeDirs = { "folder1", "folder2" };
CTM_MOCK.setupForMocking(MockFilesystem::all_directories, fakeDirs);

// Use in your tests
MetalFury::Core::PluginSystem<MockFilesystem> pluginSystem;
auto dirs = pluginSystem.getPluginsInDirectory();
Advanced Features
Matcher Statements: Customize return behavior based on input arguments.
```
Custom Return Queues: Control ordered outputs across multiple calls.

Invocation Tracking: Automatically record call counts per method.

Fine-grained control without losing performance.

# Installation
CTM is header-only.

Include the core .h file(s) in your project.

No library linking required.

No dependencies outside the C++ standard library.

```
#include "CompileTimeMocking.h" // Adjust the path as necessary
```

# Ideal Use Cases
CTM is a great fit for:

Legacy C++ projects where refactoring for interfaces isn't practical.

Embedded systems or low-latency applications where runtime costs must be minimized.

Testing stateless utilities or static systems cleanly.

Compile-time analysis and validation of mock structure.

# When to Consider Traditional Frameworks
While CTM covers many use cases, you might prefer frameworks like gMock or Trompeloeil if:

You need to mock private or protected methods.

You require full dynamic behavior or test doubles created at runtime.

You prefer mocking based on abstract interfaces with rich runtime expectations.

CTM and traditional frameworks serve different needs — it's all about choosing the best tool for your project.

# Roadmap
 Expand built-in matchers (IsGreaterThan, IsLessThan, etc.)
 Add support for mock actions (e.g., throw, invoke callbacks)
 Simplify setup macros even further


# License
MIT

If you have ideas for new matchers, matcher statements, better ergonomics, or integrations, please open an Issue or PR.

⭐ Star this repository if you find CTM useful!
Together we can grow a modern alternative for C++ testing, one compile-time mock at a time.
