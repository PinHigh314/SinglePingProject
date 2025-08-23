# Build Experience & Engineering Best Practices

This document serves as the definitive guide for the project's development environment, build processes, and engineering standards. All developers and AI assistants **must** adhere to these principles to ensure consistency, reliability, and quality.

---

## 1. Firmware Build Environment (nRF / Zephyr)

### The Problem: Fragile, Terminal-Dependent Builds

Initial builds for the nRF54 firmware were failing in standard terminals. The process relied on the developer manually opening a special, pre-configured terminal from the nRF Connect Toolchain Manager. This is unreliable and prone to human error.

### The Solution: Self-Sufficient Build Scripts

The `build_firmware.sh` script was modified to be self-sufficient. It now contains a configuration variable (`NCS_DIR`) pointing to the nRF Connect SDK installation and automatically sources the `zephyr-env.sh` script at runtime.

### **Rule: Build scripts MUST NOT depend on a manually prepared terminal.**
- **DO:** Make scripts self-contained by having them set up their own environment.
- **DON'T:** Assume the user will open a specific type of terminal.

---

## 2. Android Build Environment (Android Studio / Gradle)

### Global Tool Access (`adb`)

- **Requirement:** To make Android tools like `adb` globally available, the `ANDROID_HOME` system environment variable must be set to the Android SDK location.
- **Implementation:** The `%ANDROID_HOME%\platform-tools` directory must be added to the system's `Path` variable.
- **Key Takeaway:** This makes `adb` accessible from any **new** terminal window (terminals opened before the change will not see it).

### The Gradle Wrapper (`gradlew`)

- **Requirement:** All Android builds **must** be executed using the local Gradle Wrapper (`./gradlew.bat` or `./gradlew`). This ensures every developer uses the exact same Gradle version, guaranteeing build consistency.
- **Best Practice:** A global Gradle installation is not required for day-to-day builds. It is only useful for regenerating the wrapper if it's deleted.
- **Rule:** The `build_android.sh` script **must** use the standard `gradle wrapper` command to generate the wrapper if it's missing. It **must not** embed the wrapper's script content directly, as this is brittle and unmaintainable.

### Critical: Gradle Memory Configuration (Added 8/22/2025)

#### The Problem: JVM Garbage Collector Thrashing
During MotoApp TMT1 v2.0 development, Gradle builds were hanging indefinitely (30+ minutes) or failing with "Daemon is stopping immediately since the JVM garbage collector is thrashing" errors. The default memory allocation (512 MiB heap, 384 MiB metaspace) is insufficient for modern Android projects.

#### The Solution: Proper JVM Memory Settings
Configure adequate memory in `gradle.properties`:

```properties
# JVM memory settings to prevent garbage collector thrashing
org.gradle.jvmargs=-Xmx2048m -Xms512m -XX:MaxMetaspaceSize=512m -XX:+UseParallelGC

# Enable Gradle daemon for faster builds
org.gradle.daemon=true

# Enable parallel builds
org.gradle.parallel=true

# Enable build caching
org.gradle.caching=true
```

#### **Rule: ALWAYS configure Gradle memory settings for Android projects**
- **DO:** Set at least 2GB heap space (`-Xmx2048m`) for Android builds
- **DO:** Enable the Gradle daemon and caching for faster subsequent builds
- **DON'T:** Rely on default memory settings - they will cause build failures
- **DIAGNOSTIC:** Check daemon logs at `~/.gradle/daemon/*/daemon-*.out.log` if builds hang

---

## 3. VS Code IDE Configuration

### The Problem: "Unable to initialize Gradle Language Server"

This error occurs when VS Code's Gradle extension cannot find or use the correct Java Development Kit (JDK) version required by the project.

### The Solution: Explicitly Configure Java Home

The Android Gradle Plugin (AGP) 8.x requires **Java 17**.

- **Rule:** The VS Code User `settings.json` file **must** be configured to point directly to a Java 17 JDK. The JDK bundled with Android Studio is the most reliable choice.

  ```json
  // Example for d:\Development\SinglePingProject\.vscode\settings.json
  {
    "java.home": "C:\\Program Files\\Android\\Android Studio\\jbr"
  }
  ```
- **Key Takeaway:** Do not rely on the system's default Java version. Explicitly configure the IDE for the project's specific requirements.

---

## 4. Code Quality and Linting

### The Guideline: Treat Lint Warnings as Bugs

The Android lint report (`lint-results-debug.html`) is a critical tool for maintaining code quality. It is not merely informational.

- **Rule:** All warnings identified in the lint report (e.g., outdated dependencies, unused resources, manifest issues, etc.) **must** be treated as high-priority tasks.
- **Action:** Fix all lint warnings to improve app stability, performance, and maintainability before considering a feature complete.

### Kotlin Type Safety (Added 8/22/2025)

#### The Problem: Type Mismatches in Mathematical Operations
Kotlin is strict about type safety. Mixing `Double` and `Float` types in calculations will cause compilation failures.

#### Example Issue from MotoApp:
```kotlin
// WRONG - Math.random() returns Double, but we're adding to Float
val noise = (Math.random() - 0.5) * 0.6
return (distance + noise).coerceIn(0.1f, 50f)  // Type mismatch error!

// CORRECT - Explicitly cast to Float
val noise = ((Math.random() - 0.5) * 0.6).toFloat()
return (distance + noise).coerceIn(0.1f, 50f)
```

#### **Rule: Be explicit with numeric type conversions**
- **DO:** Use `.toFloat()`, `.toDouble()`, etc. when mixing numeric types
- **DO:** Use type-specific literals (e.g., `2.0f` for Float, `2.0` for Double)
- **DON'T:** Assume automatic type coercion like in Java

---

## 5. Build Automation and Project Rules

### The Mandate: Automate All Rules

The `rules.txt` file defines mandatory project processes, such as archiving build artifacts.

- **Rule:** Any rule related to the build process **must** be automated directly within the corresponding build script (`build_firmware.sh`, `build_android.sh`).
- **Implementation:** The scripts have been updated with `archive_firmware()` and `archive_apk()` functions to enforce the versioning and copying of compiled code automatically.
- **Key Takeaway:** This is the only way to guarantee 100% compliance. The `rules.txt` file documents the *why*, and the scripts automate the *how*.

---

## 6. Android Resource Management (Added 8/22/2025)

### The Problem: Missing Launcher Icons Cause Build Failures

Android apps require launcher icons referenced in `AndroidManifest.xml`. Missing these resources results in AAPT (Android Asset Packaging Tool) errors during the build process.

### The Solution: Adaptive Icons with Vector Drawables

Modern Android apps should use adaptive icons that work across different device shapes and themes:

1. **Create vector drawable foreground** (`res/drawable/ic_launcher_foreground.xml`)
2. **Define adaptive icon XML** (`res/mipmap-anydpi-v26/ic_launcher.xml` and `ic_launcher_round.xml`)
3. **Add background color** to `res/values/colors.xml`

#### Example Adaptive Icon Structure:
```xml
<!-- res/mipmap-anydpi-v26/ic_launcher.xml -->
<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">
    <background android:drawable="@color/ic_launcher_background"/>
    <foreground android:drawable="@drawable/ic_launcher_foreground"/>
</adaptive-icon>
```

#### **Rule: Always create launcher icons before first build**
- **DO:** Use adaptive icons for Android 8.0+ compatibility
- **DO:** Use vector drawables for scalability
- **DON'T:** Reference non-existent mipmap resources in AndroidManifest.xml
- **QUICK FIX:** Create placeholder icons if design assets aren't ready

---

## 7. Dependency Management Best Practices (Added 8/22/2025)

### The Problem: Alpha/Beta Library Versions Cause Build Failures

During MotoApp development, the Vico charting library (alpha version 2.0.0-alpha.19) caused dependency resolution failures and build issues.

### The Solution: Prefer Stable Libraries or Custom Implementations

When the Vico library failed, a custom Canvas-based graph implementation was created instead, which:
- Eliminated external dependency issues
- Provided full control over the visualization
- Reduced APK size
- Improved build reliability

#### **Rule: Avoid alpha/beta dependencies in production code**
- **DO:** Use stable library versions whenever possible
- **DO:** Consider custom implementations for simple visualizations
- **DO:** Test dependency updates in isolation before committing
- **DON'T:** Use alpha/beta libraries unless absolutely necessary
- **FALLBACK:** If unstable libraries are required, document the risks and have a backup plan

### Example: Custom Graph Implementation
Instead of complex charting libraries, a simple Canvas-based solution often suffices:
```kotlin
Canvas(modifier = Modifier.fillMaxSize()) {
    // Draw grid, axes, and data points directly
    // Full control, no dependency issues
}
```

---

## 8. Terminal Compatibility for Cross-Platform Development (Added 8/22/2025)

### The Problem: PowerShell vs Bash Command Incompatibility

During MotoApp development, commands that worked in PowerShell failed when the terminal switched to Git Bash (MINGW64). For example:
- PowerShell: `Get-Process | Where-Object {$_.ProcessName -like "*java*"}`
- Git Bash: `ps aux | grep -E "java|gradle"`

### The Solution: Use Git Bash for Gradle Commands

Git Bash (MINGW64) provides better compatibility with Gradle wrapper scripts and Unix-style commands commonly used in Android development.

#### **Rule: Use Git Bash for Android/Gradle builds on Windows**
- **DO:** Use Git Bash (MINGW64) for running `./gradlew` commands
- **DO:** Use Unix-style commands (`ls`, `cp`, `mkdir -p`) in Git Bash
- **DON'T:** Mix PowerShell and Bash commands in the same session
- **TIP:** VS Code may switch terminals automatically - check the prompt

### Terminal Identification:
- **PowerShell:** Prompt shows `PS C:\path>`
- **Git Bash:** Prompt shows `user@COMPUTER MINGW64 /c/path`
- **Command Prompt:** Prompt shows `C:\path>`

---

## 9. Troubleshooting Build Issues (Added 8/22/2025)

### Common Android Build Problems and Solutions

#### Problem: Build Hangs Indefinitely
**Symptoms:** Gradle process runs but makes no progress
**Solution:** 
1. Kill Java/Gradle processes: `Stop-Process -Name java -Force`
2. Clear Gradle cache: `./gradlew clean`
3. Increase memory in `gradle.properties`
4. Check daemon logs for errors

#### Problem: "Could not resolve dependencies"
**Symptoms:** Gradle cannot download or resolve library dependencies
**Solution:**
1. Check internet connectivity
2. Try offline mode if dependencies are cached: `./gradlew assembleDebug --offline`
3. Clear dependency cache: `rm -rf ~/.gradle/caches/`
4. Verify repository URLs in `build.gradle`

#### Problem: Resource Compilation Errors
**Symptoms:** AAPT errors about missing resources
**Solution:**
1. Verify all referenced resources exist
2. Check for typos in resource names
3. Ensure proper resource qualifiers (e.g., `-v26` for API 26+)
4. Clean and rebuild: `./gradlew clean assembleDebug`

### **Rule: Document and share build issue solutions**
- **DO:** Keep logs of build errors and their solutions
- **DO:** Update this document when new issues are discovered
- **DO:** Check build reports at `build/reports/` for detailed error information
- **DON'T:** Ignore warning messages - they often precede failures

---

## Summary of Critical Rules

1. **Memory Configuration is Mandatory** - Always configure Gradle JVM memory settings
2. **Type Safety is Non-Negotiable** - Be explicit with Kotlin type conversions
3. **Resources Must Exist** - Create all referenced resources before building
4. **Prefer Stable Dependencies** - Avoid alpha/beta libraries in production
5. **Document Everything** - Update this guide with new discoveries
6. **Automate Compliance** - Build scripts must enforce all project rules

This document is a living guide. Update it whenever new build issues are encountered and resolved.
