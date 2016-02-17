# packr

__Public Service Announcement: With packr v2.0, command line interfaces to both packr.jar and the native executable have changed! If you upgrade from a previous version, please read this documentation again to make sure you've updated your configuration(s) accordingly.__

## About

Packages your JAR, assets and a JVM for distribution on Windows, Linux and Mac OS X, adding a native executable file to make it appear like a native app. Packr is most suitable for GUI applications, such as games made with [libGDX](http://libgdx.badlogicgames.com/).

#### [Download Packr](http://bit.ly/packrgdx)

## Usage

You point packr at your JAR file(s) containing your code and assets, some configuration parameters, and a URL or local file location to a JDK build for your target platform.

Invoking packr from the command line may look like this:

```bash
java -jar packr.jar \
     --platform mac \
     --jdk openjdk-1.7.0-u45-unofficial-icedtea-2.4.3-macosx-x86_64-image.zip \
     --executable myapp \
     --classpath myapp.jar \
     --mainclass com.my.app.MainClass \
     --vmargs Xmx1G \
     --resources src/main/resources path/to/other/assets \
     --minimizejre soft \
     --output out-mac
```

| Parameter | Meaning |
| --- | --- |
| platform | one of "windows32", "windows64", "linux32", "linux64", "mac" |
| jdk | ZIP file location or URL to an OpenJDK build containing a JRE. Prebuild JDKs can be found at https://github.com/alexkasko/openjdk-unofficial-builds |
| executable | name of the native executable, without extension such as ".exe" |
| classpath | file locations of the JAR files to package |
| mainclass | the fully qualified name of the main class, using dots to delimit package names |
| vmargs | list of arguments for the JVM, without leading dashes, e.g. "Xmx1G" |
| resources (optional) | list of files and directories to be packaged next to the native executable |
| minimizejre | minimize the JRE by removing directories and files as specified by the config file. Comes with two config files out of the box called "soft" and "hard". See below for details on the minimization config file. |
| output | the output directory |
| icon (optional, OS X) | location of an AppBundle icon resource (.icns file) |
| bundleidentifier (optional, OS X) | the bundle identifier of your Java application, e.g. "com.my.app" |

Alternatively, you can put all the command line arguments into a JSON file which might look like this:

```json
{
    "platform": "mac",
    "jdk": "/Users/badlogic/Downloads/openjdk-1.7.0-u45-unofficial-icedtea-2.4.3-macosx-x86_64-image.zip",
    "executable": "myapp",
    "classpath": [
        "myapp.jar"
    ],
    "mainclass": "com.my.app.MainClass",
    "vmargs": [
       "Xmx1G"
    ],
    "resources": [
        "src/main/resources",
        "path/to/other/assets"
    ],
    "minimizejre": "soft",
    "outdir": "out-mac"
}
```

You can then invoke the tool like this:

```bash
java -jar packr.jar my-packr-config.json
```

It is possible to combine a JSON configuration and the command line. For single options, the command line parameter overrides the equivalent JSON option. For multi-options (e.g. "classpath" or "vmargs"), the options are merged.

This is an example which overrides the output folder, and adds another VM argument (note that the config file name is delimited with "--" because the option prior to it, "--vmargs", allows multiple arguments):

```bash
java -jar packr.jar --output target/out-mac --vmargs Xms256m -- my-packr-config.json
```

Finally, you can use packr from within your code. Just add the JAR file to your project, either manually, or via the following Maven dependency:

```xml
<dependency>
   <groupId>com.badlogicgames.packr</groupId>
   <artifactId>packr</artifactId>
   <version>2.0</version>
</dependency>
```

To invoke packr, you need to create an instance of `PackrConfig` and pass it to `Packr.pack()`:

```java
PackrConfig config = new PackrConfig();
config.platform = PackrConfig.Platform.Windows32;
config.jdk = "/User/badlogic/Downloads/openjdk-for-mac.zip";
config.executable = "myapp";
config.classpath = Arrays.asList("myjar.jar");
config.mainClass = "com.my.app.MainClass";
config.vmArgs = Arrays.asList("Xmx1G");
config.minimizeJre = "soft";
config.outDir = "out-mac";

new Packr().pack(config);
```

## Minimization

A standard OpenJDK JRE weighs about 90mb unpacked and about 50mb packed. Packr helps you cut down on that size, thus also reducing the download size of your app.

To minimize the JRE that is bundled with your app, you have to specify a minimization configuration file via the `minimizejre` flag you supply to Packr. Such a minimization configuration contains the names of files and directories within the JRE to be removed, one per line in the file. E.g.:

```
jre/lib/rhino.jar
jre/lib/rt/com/sun/corba
```

This will remove the rhino.jar (about 1.1MB) and all the packages and classes in com.sun.corba from the rt.jar file. To specify files and packages to be removed from the JRE, simply prepend them with `jre/lib/rt/`.

Packr comes with two such configurations out of the box, [`soft`](https://github.com/libgdx/packr/blob/master/src/main/resources/minimize/soft) and [`hard`](https://github.com/libgdx/packr/blob/master/src/main/resources/minimize/hard)

Additionally, Packr will compress the rt.jar file. By default, the JRE uses zero-compression on the rt.jar file to make application startup a little faster.

## Output

When packing for Windows, the following folder structure will be generated

```
outdir/
   executable.exe
   yourjar.jar
   config.json
   jre/
```

Linux

```
outdir/
   executable
   yourjar.jar
   config.json
   jre/
```

Mac OS X

```
outdir/
   Contents/
      Info.plist
      MacOS/
         executable
         yourjar.jar
         config.json
         jre/
      Resources/
         icons.icns [if config.icon is set]
```

You can further modify the Info.plist to your liking, e.g. add icons, a bundle identifier etc. If your `outdir` has the `.app` extension it will be treated as an application bundle by Mac OS X.

## Executable command line interface

By default, the native executables forward any command line parameters to your Java application's main() function. So, with the configurations above, `./myapp -x y.z` is passed as `com.my.app.MainClass.main(new String[] {"-x", "y.z" })`.

The executables themselves expose an own interface, which has to be enabled explicitly by passing `-c` or `--cli` as the **very first** parameter. In this case, the special delimiter parameter `--` is used to separate the native CLI from parameters to be passed to Java. In this case, the example above would be equal to `./myapp -c [arguments] -- -x y.z`.

Try `./myapp -c --help` for a list of available options. They are also listed [here](https://github.com/libgdx/packr/blob/master/src/main/native/README.md#command-line-interface).

> Note: On Windows, the executable does not show any output by default. Here you can use `myapp.exe -c --console [arguments]` to spawn a console window, making terminal output visible.

## Building from source code

If you want to modify the Java code only, it's sufficient to invoke Maven.

```
mvn clean package
```

This will create a `packr-VERSION.jar` file in `target` which you can invoke as described in the Usage section above.

If you want to compile the native executables, please follow [these instructions](https://github.com/libgdx/packr/blob/master/src/main/native/README.md). Each of the build scripts will create executable files for the specific platform and copy them to src/main/resources.

## Limitations

  * Icons aren't set yet on Windows and Linux, you need to do that manually.
  * Minimum platform requirement on MacOS is OS X 10.7.
  * JRE minimization is very conservative. Depending on your app, you can carve out stuff from a JRE yourself, disable minimization and pass your custom JRE to packr.
  * On MacOS, the JVM is spawned in its own thread by default, which is a requirement of AWT. This does not work with code based on LWJGL3/GLFW, which needs the JVM be spawned on the main thread. You can enforce the latter with adding the `-XstartOnFirstThread` VM argument to your MacOS packr config.

## License & Contributions

The code is licensed under the [Apache 2 license](http://www.apache.org/licenses/LICENSE-2.0.html). By contributing to this repository, you automatically agree that your contribution can be distributed under the Apache 2 license by the author of this project. You will not be able to revoke this right once your contribution has been merged into this repository.

## Security

Distributing a bundled JVM has security implications, just like bundling any other runtimes like Mono, Air, etc. Make sure you understand the implications before deciding to use this tool. Here's a [discussion on the topic](http://www.reddit.com/r/gamedev/comments/24orpg/packr_package_your_libgdxjavascalajvm_appgame_for/ch99zk2).
