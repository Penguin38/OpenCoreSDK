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
```
dependencies {
    ...
    implementation 'com.github.Penguin38:OpenCoreSDK:opencore-1.2.4'
}
```
## 例子
```
... {
    //  初始化组件
    Coredump.getInstance().init();
   
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
