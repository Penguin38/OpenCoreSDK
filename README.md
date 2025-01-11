# 技术体系
![core-analysis](https://raw.githubusercontent.com/Penguin38/OpenCoreAnalysisKit/refs/heads/main/doc/OpenCoreAnalyzer.jpg)

| Project      | Path                                              |
|:------------:|---------------------------------------------------|
|core-parser   | https://github.com/Penguin38/OpenCoreAnalysisKit  |
|linux-parser  | https://github.com/Penguin38/OpenLinuxAnalysisKit |
|crash-android | https://github.com/Penguin38/crash-android        |
|OpenCoreSDK   | https://github.com/Penguin38/OpenCoreSDK          |

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
    implementation 'com.github.Penguin38:OpenCoreSDK:opencore-1.4.7'
}
```
## 例子
```
{
    //  初始化组件
    Coredump.getInstance().init();

    //  设置超时时间 (单位秒)
    Coredump.getInstance().setCoreTimeout(Coredump.DEF_TIMEOUT);

    //  设置 Coredump 文件名规则
    Coredump.getInstance().setCoreFlag(Coredump.FLAG_CORE
                                     | Coredump.FLAG_PROCESS_COMM
                                     | Coredump.FLAG_PID
                                     | Coredump.FLAG_THREAD_COMM
                                     | Coredump.FLAG_TID
                                     | Coredump.FLAG_TIMESTAMP);
    
    //  设置过滤条件
    Coredump.getInstance().setCoreFilter(Coredump.FILTER_SPECIAL_VMA
                                      // | Coredump.FILTER_FILE_VMA
                                      // | Coredump.FILTER_SHARED_VMA
                                       | Coredump.FILTER_SANITIZER_SHADOW_VMA
                                       | Coredump.FILTER_NON_READ_VMA
                                       | Coredump.FILTER_SIGNAL_CONTEXT);

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
