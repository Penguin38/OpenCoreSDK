# OpenCoreSDK
## 构建项目依赖
```
allprojects {
    repositories {
        ...
        maven { url 'https://jitpack.io' }
    }
}
```
or
```
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        ...
        maven { url 'https://jitpack.io' }
    }
}
```
```
dependencies {
    ...
    implementation 'com.github.Penguin38:OpenCoreSDK:opencore-1.3.1'
}
```
## 例子
```
{
    //  初始化组件
    Coredump.getInstance().init();

    //  设置模式
    // Coredump.getInstance().setCoreMode(Coredump.MODE_PTRACE | Coredump.MODE_COPY);
    // Coredump.getInstance().setCoreMode(Coredump.MODE_PTRACE);
    // Coredump.getInstance().setCoreMode(Coredump.MODE_COPY);
    Coredump.getInstance().setCoreMode(Coredump.MODE_COPY2);


    //  设置 Coredump 保存目录
    Coredump.getInstance().setCoreDir(...);
   
    //  Java Crash 生成 Coredump
    Coredump.getInstance().enable(Coredump.JAVA);
    
    //  Native Crash 生成 Coredump
    Coredump.getInstance().enable(Coredump.NATIVE);
    
    //  设置监听器
    Coredump.getInstance().setListener(new Coredump.Listener() {
        @Override
        public void onCompleted(String path) {
                // do anything
        }
    });

    //  主动生成当前时刻 Coredump
    Coredump.getInstance().doCoredump();
}
```
