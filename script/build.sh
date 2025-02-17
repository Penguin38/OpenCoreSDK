if [ -z $JAVA_HOME ];then
    echo "JAVA_HOME is not set"
    echo "Example:"
    echo "    export JAVA_HOME=JDK_DIR"
    echo "    ./script/build.sh"
    exit
fi
./gradlew clean -Pgroup=com.github.Penguin38 -Pversion=opencore-1.4.14 -xtest -xlint assemble publishToMavenLocal
