--- /tmp/orig_main.cpp	2026-09-21 18:18:11.502656038 +0000
+++ profiles/vcs/host/main.cpp	2026-09-21 18:18:04.679593240 +0000
@@ -28,6 +28,10 @@
 #include <windows.h>
 #endif
 
+#if defined(__SWITCH__)
+#include <switch.h>
+#endif
+
 namespace {
 
 #ifdef _WIN32
@@ -158,9 +162,25 @@
 #ifdef _WIN32
     SetUnhandledExceptionFilter(&vcs_unhandled_exception_filter);
 #endif
+#if defined(__SWITCH__)
+    // Debug-only: redirects stdout/stderr over the network so
+    // `nxlink -s VCSNative.nro` from a PC on the same network shows console
+    // output live. Without this, every std::cerr write below (including the
+    // catch block's error message) goes nowhere visible on real hardware --
+    // that's why earlier failures produced no message and no log file.
+    socketInitializeDefault();
+    nxlinkStdio("stdout");
+    std::cerr << "[switch] argc=" << argc << "\n";
+    for (int i = 0; i < argc; ++i) {
+        std::cerr << "[switch] argv[" << i << "]=" << (argv[i] != nullptr ? argv[i] : "(null)") << "\n";
+    }
+#endif
     try {
         const std::filesystem::path executable_directory =
             native_executable_directory(argc > 0 ? argv[0] : nullptr);
+#if defined(__SWITCH__)
+        std::cerr << "[switch] executable_directory=" << executable_directory.string() << "\n";
+#endif
         const vcs::BootstrapPaths paths =
             vcs::resolve_bootstrap_paths(argc, argv, executable_directory);
         const std::filesystem::path &executable = paths.psp_executable;
