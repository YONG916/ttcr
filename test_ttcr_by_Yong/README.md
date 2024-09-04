# 射线追踪项目ttcr

> - 源代码链接：[Github链接](https://github.com/groupeLIAMG/ttcr)
> - 源代码：C++
> - 测试demo链接：
> - 测试环境：使用脚本文件`ps1`执行批量的参数测试；使用`python`执行测试结果可视化。
> - 电脑配置： 64 位 Windows 10 专业版操作系统，Intel(R) Core(TM) i5-11400 CPU @ 2.60GHz（6 核心 12 线程），内存大小为 16GB。



## 1. 克隆Github库

- 搜索`CMakeLists.txt`

![image-20240902111206714](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021112832.png)

- 下载`CMakeList.txt`所在文件夹的所有内容.



## 2.下载 `CMake`与相关的库

### 2.1. boost

- 直接在官网下载 boost1.8.2 [csdn链接](https://blog.csdn.net/zixingcai/article/details/141288833)

- 将下载的文件`boost_1_82_0`， 与Github下载了 `..\ttcr\boost_1_81_0`替换 

  > - 原因：测试发现原库文件`..\ttcr\boost_1_81_0`不可用；

### 2.2. engine

- 不需要下载（ 上一步 Github下载了 `..\ttcr\engine-3.4.0`）

### 2.3. vtk & CMake

- 按照这个链接里面的教程， 下载 `CMake` & `Vtk`

  [csdn链接](https://blog.csdn.net/qq_31461977/article/details/133974626)



## 3.通过CMake生成`.sln`文件

### 3.1.重写`CMakeLists.txt`

```cmake
cmake_policy(SET CMP0167 OLD)
cmake_minimum_required(VERSION 3.5) # 该项目所需最低的CMake版本号, 至少是3.5

project(ttcr) # 生成的项目名称, ttcr.sln

# VTK库
add_definitions(-DVTK)
set( VTK_DIR /usr/local/VTK/lib/cmake/vtk-7.0 )
find_package(VTK 7.0 REQUIRED NO_MODULE)
message(STATUS "VTK_LIBRARIES are set to: ${VTK_LIBRARIES}")

# 修改 Boost
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_INCLUDE_DIRS ".../boost_1_82_0") # 手动设置路径
include_directories(${Boost_INCLUDE_DIRS})

# 设置 Boost 库路径（lib 文件夹路径）
# 手动指定需要的库文件
set(Boost_LIBRARIES,".../boost_1_82_0/lib64-msvc-14.3") # 手动设置路径

# 手动设置 Boost_FOUND 为 TRUE
set(Boost_FOUND TRUE)

# 如果找到 Boost，则将 Boost 的头文件路径添加到项目中
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
else()
    message(FATAL_ERROR "Boost not found!")
endif()


# 设置 Eigen 库的路径
set(EIGEN3_INCLUDE_DIR ".../eigen-3.4.0") # 手动设置路径
include_directories(${EIGEN3_INCLUDE_DIR})
if(NOT EIGEN3_INCLUDE_DIR)
    message(FATAL_ERROR "Please point the environment variable EIGEN3_INCLUDE_DIR to the include directory of your Eigen3 installation.")
endif()
include_directories("${EIGEN3_INCLUDE_DIR}")

# 设置 
set(CMAKE_CXX_FLAGS "-std=c++14 -march=native")
set(CMAKE_INSTALL_PREFIX $ENV{HOME})

set(ttcr3d_SRCS ttcr3d.cpp ttcr_io.cpp)
set(ttcr2d_SRCS ttcr2d.cpp ttcr_io.cpp)
set(ttcr2ds_SRCS ttcr2ds.cpp ttcr_io.cpp)

set(CMAKE_VERBOSE_MAKEFILE on)

add_executable(ttcr3d ${ttcr3d_SRCS}) # 生成可执行文件
add_executable(ttcr2d ${ttcr2d_SRCS}) # 生成可执行文件
add_executable(ttcr2ds ${ttcr2ds_SRCS}) # 生成可执行文件

# 链接 VTK 和 Boost 库
target_link_libraries(ttcr3d ${VTK_LIBRARIES} ${Boost_LIBRARIES}) # 链接
target_link_libraries(ttcr2d ${VTK_LIBRARIES} ${Boost_LIBRARIES}) # 链接
target_link_libraries(ttcr2ds ${VTK_LIBRARIES} ${Boost_LIBRARIES}) # 链接

set_property(TARGET ttcr3d ttcr2d ttcr2ds PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
install(TARGETS ttcr3d ttcr2d ttcr2ds RUNTIME DESTINATION bin)
```

- 修改完保存。



### 3.2.在ttcr文件夹下生成`.sln`文件

```cmd
mkdir build # 创建文件夹
cd build # 进入文件夹
cmake .. # 编译CMakeList.txt文件
```

- 编译成功：

<img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202408281543095.png" alt="image-20240828154339947" style="zoom: 150%;" />

- 生成`.sln`文件

  ![image-20240828154442575](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202408281544643.png)

- 双击 `ttcr.sln` 文件，这将会打开 Visual Studio 并加载项目。




## 4.VS2022的设置

### 4.1. 检查是否成功导入库

- 每个可执行项目下都要检查：Properties ➜ C/C++ ➜ General ➜ Additional include Directories

<img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021445132.png" alt="image-20240902144531088" style="zoom: 33%;" />

- 检查是否正确添加了`boost`、`eigen`



### 4.2. 检查与修改 Command line

- Properties ➜ C/C++ ➜ General ➜ Command line

  <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021449659.png" alt="image-20240902144919618" style="zoom: 33%;" />

- 不添加 `/bigobj`就会报错。



### 4.3. 检查 & 修改编译选项

- 参考：[CSDN](https://blog.csdn.net/kevin_lp/article/details/134802664?spm=1001.2101.3001.6661.1&utm_medium=distribute.pc_relevant_t0.none-task-blog-2~default~BlogCommendFromBaidu~Rate-1-134802664-blog-6385304.235%5Ev43%5Epc_blog_bottom_relevance_base5&depth_1-utm_source=distribute.pc_relevant_t0.none-task-blog-2~default~BlogCommendFromBaidu~Rate-1-134802664-blog-6385304.235%5Ev43%5Epc_blog_bottom_relevance_base5&utm_relevant_index=1)

<img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202408292119008.png" alt="image-20240829211917800" style="zoom: 25%;" />

- 如果不修改， 就会出现Link问题：

  ![image-20240829193231861](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202408291932007.png)



### 4.4. `ttcr_t.h`出现 `small`关键字定义变量的报错问题

- 修改如下：

  <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021515287.png" alt="image-20240902151557231" style="zoom: 25%;" />



## 5.修改`.cpp`文件

### 5.1. 修改项目的输出目录和中间目录

> [CSDN链接](https://blog.csdn.net/RQ997832/article/details/123698874)

- 为了方便中间文件和输出文件管理, 修改文件路径：

<img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021535277.png" alt="image-20240902153515218" style="zoom:33%;" />

- Output Directory: `$(SolutionDir)bin\$(Platform)\$(Configuration)\` 

  > 在两个项目后面再新增一个文件夹：
  >
  > - ttrc2d: ` 2D_builder\`
  > - ttrc3d: `3D_builder\`

- Intermediate Directory:  `$(SolutionDir)bin\intermediates\$(Platform)\$(Configuration)\`

  

### 5.2. 修改ttcr2d.cpp 和 ttcr3d.cpp文件

#### 5.1.1. 头文件缺少的报错问题

```c++
#include <ciso646> // 由于缺少这个头文件，‘and’ 无法识别，已一直报错 
```



#### 5.1.2. 修改输入输出文件夹

- 为了清晰直观地**测试**输入输出文件， 在`.exe`文件的目录中增加`..\input` 和 `..\output`两个文件夹如下：

  <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021545104.png" alt="image-20240902154552040" style="zoom:50%;" />

- 将输入文件放在`..\input`文件夹下

  <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021609477.png" alt="image-20240902160911405" style="zoom:50%;" />

- `.par`文件与`.exe`放在同一文件下

  <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021610330.png" alt="image-20240902161000266" style="zoom:50%;" />

  

- 在原`.cpp` 文件中修改输入输出路径, 在`int main`函数中找到：

  ```c++
  if ( par.singlePrecision ) {
      return body<float>(par);
  } else {
      return body<double>(par);
  }
  ```

  在这段代码之前新增代码如下：
  ```c++
  
  /*******修改输入 & 输出路径***********/
  
  // S1. 子文件路径
  std::string input_folder = "./input/";
  std::string output_folder = "./output/";
  
  // S2. 修改 par 中的路径
  par.basename = output_folder + par.basename;
  par.modelfile = input_folder + par.modelfile;
  par.slofile = input_folder + par.slofile;
  
  // S3. 处理 srcfiles 列表中的每个文件路径
  for (size_t i = 0; i < par.srcfiles.size(); ++i) {
      par.srcfiles[i] = input_folder + par.srcfiles[i];
  }
  
  par.rcvfile = input_folder + par.rcvfile;
  // S4. 打印&检查文件路径
  cout << "par.basename: " << par.basename << "\n";
  cout << "par.slofile: " << par.slofile << "\n";
  cout << "par.srcfiles:\n";
  for (const auto& file : par.srcfiles) {
      cout << file << "\n";
  }
  cout << "par.rcvfile: " << par.rcvfile << "\n";
  cout << "\n";  // 额外的换行符
  /********************************************************/
  ```

  

### 5.2. 修改ttcr_io.cpp

#### 5.2.1. 头文件在window不可用的问题

- 头文件中， 注释下面代码

  ```c++
  extern "C" {
  #include <unistd.h> // for getopt, 这个无法在window下执行
  }	
  ```

  - 该文件定义了许多与系统调用和操作有关的接口，例如文件操作、进程控制和进程间通信。
  - 注释原因： 与 Unix-like 系统（比如 Linux 和 macOS）相关， 因此这个头文件在 **Windows 系统的标准库中通常不可用**。

  

- 替代方案：下载`getopt.h` ([CSDN链接](https://blog.csdn.net/qq_33835370/article/details/108874699))

  ```c++
  #include <getopt.h> // for getopt，window可用
  ```

  

### 5.3.生成可执行文件`.exe`

#### 5.3.1. ttcr2d

![image-20240902170055196](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021700274.png)

#### 5.3.2.ttcr3d

![image-20240902162551677](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021625742.png)



## 6.简单测试的例子

> - 注意：每一次编译错误， 最好先`clean`， 然后再重新编译。

### 6.1.  准备测试数据

#### 6.1.1. 数据说明

> - Github中的参数说明：[Github](https://github.com/groupeLIAMG/ttcr/blob/master/docs/command_line.md)
> - 默认是快速扫描法（FSM）

| 输入文件                                  | 内容                                                         |
| ----------------------------------------- | ------------------------------------------------------------ |
| model2d.par                               | <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021614849.png" alt="image-20240902161414788" style="zoom:50%;" /> |
| model2d.grd                               | <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021615801.png" alt="image-20240902161518751" style="zoom:50%;" /> |
| model2d.slo<br />沿z轴， 叠加排序为一整列 | <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021616014.png" alt="image-20240902161600958" style="zoom: 50%;" /> |
| src.dat                                   | <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021619545.png" alt="image-20240902161920481" style="zoom:50%;" /> |
| rcv.dat                                   | <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021619213.png" alt="image-20240902161931159" style="zoom:50%;" /> |



#### 6.1.2.数据放置的文件夹位置

- `.par`文件与`.exe`放在同一文件下

  <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021610330.png" alt="image-20240902161000266" style="zoom:50%;" />

- 其余文件放在`..\input`文件夹下

  <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021609477.png" alt="image-20240902160911405" style="zoom:50%;" />



### 6.2. 执行测试

#### 6.2.1. 执行指令说明

> [Github](https://github.com/YONG916/ttcr/blob/master/docs/command_line.md)

```tex
-p 指定参数文件（必选）
-h 显示简短的帮助信息
-k 以 VTK 格式保存模型
-v 详细模式
-t 计算构建网格和执行光线追踪的时间
-s 将次级节点坐标导出为 ASCII 文件（仅适用于 (D)SPM 方法）
```



#### 6.2.2. 具体执行

- Step 1

  > ![image-20240902162357230](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021623303.png)

- Step2. 输入如下指令

  ```cmd
  ttcr2d -v -p model2d.par -t -k -s
  ```

- Step 3. 执行结果

  1. 执行报告：

     <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021645838.png" alt="image-20240902164533098" style="zoom:50%;" />

  2. 输出文件：

     <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021649597.png" alt="image-20240902164905536" style="zoom: 67%;" />

​	

## 7.改变输入参数,  测试运行时间

> - 通过 PowerShell 脚本文件`.ps1`执行测试; 
>
> - 在`.exe`文件夹， 构建新的文件夹`TestScripts`
>
>   <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021702494.png" alt="image-20240902170255420" style="zoom:50%;" />
>
> - 打开 `Window Powershell`
>
>   <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021751341.png" alt="image-20240902175104248" style="zoom:50%;" />



### 7.1.  二维模型测试

#### 7.1.1.测试最短路径算法的参数：次级网格节点（secondary nodes）

##### 7.1.1.1. 初始参数设置

> - 算法：最短路径算法（SPM）,  只针对SPM有该参数。
>
> - 网格分辨率：（200， 100）
>
> - 源点个数：1
>
> - 接收点个数：20
>
> - 2D层状的慢度：
>
>   ```python
>   # 设置慢度值
>   6.666666666666666444e-04  # 0-20层
>   4.000000000000000192e-04  # 20-40层
>   2.857142857142857357e-04  # 40-100层
>   ```



##### 7.1.1.2.编写脚本文件`TestSecondaryNodes.ps1`

- 功能：输入 secondary nodes 为 0到10，输出运行时间。

```powershell
# 定义可执行文件的路径
$executable = ".\ttcr2d"  # 使用相对路径或完整路径
$configFile = ".\model2d.par"  # 使用相对路径或完整路径

# 先返回到上一级文件夹
Set-Location ..  # 这会将当前目录设置为上一级文件夹

# 初始化一个空的表格（对象数组）
$results = @()

# 循环遍历 secondary nodes 的值从 0 到 10
for ($sec_nodes = 0; $sec_nodes -le 10; $sec_nodes++) {
    # 更新配置文件中的 secondary nodes 参数
    (Get-Content $configFile) -replace '^\d+\s+# secondary nodes', "$sec_nodes             # secondary nodes" | Set-Content $configFile

    # 运行程序并记录运行时间
    Write-Output "Running with secondary nodes = $sec_nodes"
    $start_time = Get-Date

    # 执行命令并捕获输出
    $output = & $executable -v -p $configFile -t -k -s

    # 提取包含“Time to build grid”和“Time to perform raytracing”的那一行
    $gridTimeLine = $output | Select-String -Pattern "Time to build grid:"
    $raytraceTimeLine = $output | Select-String -Pattern "Time to perform raytracing:"

    # 从中提取实际的时间数值
    $gridTime = $gridTimeLine -replace '.*Time to build grid:\s+([0-9.]+).*','$1'
    $gridTime = [double]::Parse($gridTime)
    
    $raytraceTime = $raytraceTimeLine -replace '.*time to perform raytracing:\s+([0-9.]+).*','$1'
    $raytraceTime = [double]::Parse($raytraceTime)

    $totalTime = $gridTime + $raytraceTime
    $singleRayPathTime = $totalTime / 20

    # 将结果保存到表格中
    $results += [PSCustomObject]@{
        SecondaryNodes = $sec_nodes
        GridTimeSeconds = $gridTime
        RaytraceTimeSeconds = $raytraceTime
        TotalTimeSeconds = $totalTime
        SingleRayPathTime = $singleRayPathTime
    }

    Write-Output "Time to build grid with secondary nodes = $sec_nodes, $gridTime seconds"
    Write-Output "Time to perform raytracing with secondary nodes = $sec_nodes, $raytraceTime seconds"
    Write-Output "Total time with secondary nodes = $sec_nodes, $totalTime seconds"
    Write-Output "---------------------------------------"
}

# 回到原来的 TestScripts 目录
Set-Location TestScripts 

# 以表格方式弹出窗口显示数据
$results | Out-GridView -Title "Runtime Results"

# 也可以将结果输出到一个 CSV 文件
$results | Export-Csv -Path ".\TestResult\runtime_results.csv" -NoTypeInformation
```



##### 7.1.1.3. 测试结果

<img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021754291.png" alt="image-20240902175415192" style="zoom:50%;" />

- 表格记录运行时间：

  ![image-20240902175917723](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021819995.png)

- 根据上面表格， 绘制折线图：

  1. 总运行时间、构建网格的时间、射线追踪运行时间：

  <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021927122.png" alt="image-20240902192750021" style="zoom:50%;" />

 2. 平均每条射线的运行时间：

    <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021952087.png" alt="image-20240902195245984" style="zoom:50%;" />

- 次级节点数量的增加会大大增加运行时间。但是，次级节点数量为0的时候，运行效果很差（如下图所示）， 分界线处没有折射。当Secondary Nodes = 1的时候， 与Secondary Nodes = 10效果类似。因此，对于不同分辨率的模型， 需要找到合适的次级节点数。

  > - Secondary Nodes = 0:
  >
  >   ![image-20240902193655855](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021936944.png)
  >
  > - Secondary Nodes = 1:
  >
  >   ![image-20240902194632552](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021946637.png)
  >
  > - Secondary Nodes = 2:
  >
  >   ![image-20240902194728953](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021947038.png)
  >
  >   ...
  >
  > - Secondary Nodes = 10:
  >
  >   ![image-20240902194128469](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409021941554.png)

  

- 接收器的初至走时：

  > <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022338207.png" alt="image-20240902233826101" style="zoom:50%;" />



#### 7.1.2.测试快速扫描法的参数：最大扫描迭代次数（max number of iteration）

##### 7.1.2.1.初始参数设置

> - 算法：快速扫描算法（FSM）,  只针对SPM有该参数。默认算法迭代次数是20
>
> - 网格分辨率：（200， 100）
>
> - 源点个数：1
>
> - 接收点个数：20
>
> - 2D层状的慢度：
>
>   ```python
>   # 设置慢度值
>   6.666666666666666444e-04  # 0-20层
>   4.000000000000000192e-04  # 20-40层
>   2.857142857142857357e-04  # 40-100层
>   ```

##### 7.1.2.2. 测试结果

![image-20240902205001959](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022050052.png)

- 根据上面表格， 绘制折线图：

  1. 总运行时间、构建网格的时间、射线追踪运行时间：

     <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022100073.png" alt="image-20240902210042938" style="zoom:50%;" />

  2. 平均每条射线的运行时间：

     <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022102731.png" alt="image-20240902210238614" style="zoom:50%;" />

- 在迭代次数在20到100的范围内， FSM方法的运行时间与迭代次数的大小关系不大， 射线路径的寻找效果差不多。

  > - Iteration = 20（默认值）:
  >
  >   ![image-20240902211354183](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022113309.png)
  >
  >   ...
  >
  > - Iteration = 100:
  >
  >   ![image-20240902205211816](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022052931.png)

- 此结果说明：`epsilon=1e-15` 已经使计算在较早的迭代次数就收敛了。 因此，不需要变化这个参数。



#### 7.1.3.测试快速扫描法的参数：是否启用三阶加权本质非振荡（WENO）算子（fsm high order）

##### 7.1.3.1.初始参数设置

> - 算法：快速扫描算法（FSM）,  只针对SPM有该参数。默认算法迭代次数是20
>
> - 网格分辨率：（200， 100）
>
> - 源点个数：1
>
> - 接收点个数：20
>
> - 2D层状的慢度：
>
>   ```python
>   # 设置慢度值
>   6.666666666666666444e-04  # 0-20层
>   4.000000000000000192e-04  # 20-40层
>   2.857142857142857357e-04  # 40-100层
>   ```

##### 7.1.3.2. 测试结果

- 不启用该算子：

  > - 运行时间：0.1163231 s
  >
  > - 射线路径：
  >
  >   ![image-20240902212540030](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022125124.png)

- 启用该算子：

  > - 运行时间: 0.900507 s 
  >
  > - 射线路径：
  >
  >   ![image-20240902213223127](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022132222.png)

- 接收器的初至走时：

  > <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022335012.png" alt="image-20240902233543896" style="zoom:50%;" />

- 此结果说明， 在二维模型下， 不启用该算子运行速度更快， 且射线路径和初至走时的结果差不多。



#### 7.1.4.测试动态最短路径算法的参数：三级节点的个数和三级节点的半径（tertiary nodes， src radius tertiary）

- **tertiary nodes**：附加在主网格或基本网格上的节点，主要用于在复杂或高精度要求的计算中增强插值和计算精度。（本实验设置取值范围：1-10）

- **src radius tertiary** :这个参数定义了在计算时，围绕光源（源点）所考虑的三级节点的半径。也就是说，它控制了在光源周围的一个圆或球（取决于二维或三维情况）内，哪些三级节点会被纳入计算。（本实验设置取值范围：1-10）

  

##### 7.1.4.1. 初始参数设置

> - 算法：动态最短路径算法（DSPM）,  只针对DSPM有该参数。网格分辨率：（200， 100）
>
> - 源点个数：1
>
> - 接收点个数：20
>
> - Secondary Nodes: 1
>
> - 2D层状的慢度：
>
>   ```python
>   # 设置慢度值
>   6.666666666666666444e-04  # 0-20层
>   4.000000000000000192e-04  # 20-40层
>   2.857142857142857357e-04  # 40-100层
>   ```

##### 7.1.4.2. 测试结果

- 最大运行时间与最小运行时间的差异：0.1862932 s

<img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022244923.png" alt="image-20240902224409602" style="zoom:50%;" />

- tertiary nodes = 10的时候的剖面图:

  > <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022247788.png" alt="image-20240902224742802" style="zoom:50%;" />

- src radius tertiary = 10 的时候的剖面图:

  > <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022248893.png" alt="image-20240902224821780" style="zoom:50%;" />

- 测试射线路径

  > - tertiary nodes = 1 , src radius tertiary = 1:
  >
  >   ![image-20240902225645495](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022256601.png)
  >
  > - tertiary nodes = 10 , src radius tertiary = 10:
  >
  >   ![image-20240902225124708](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022251814.png)

- 接收器的初至走时：

  > <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022339021.png" alt="image-20240902233916917" style="zoom:50%;" />
  >
  > <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409022318033.png" alt="image-20240902231819913" style="zoom:50%;" />

- 初至走时的差异不到0.001， 说明`tertiary nodes = 1 , src radius tertiary = 1`是可行的， 并且运行时间更短。



#### 7.1.5. 总结

- 在结果相对较好的参数下， 算法运行时间对比：

  |                               | SPM<br /> (secondary nodes = 1) | FSM <br />(fsm high order = 0) | DSPM<br /> (tertiary nodes = 1 , src radius tertiary = 1) |
  | ----------------------------- | ------------------------------- | ------------------------------ | --------------------------------------------------------- |
  | Running Time<br />（Seconds） | 0.4819615                       | **0.1163231**                  | 0.4552224                                                 |

- 最短路径算法（SPM）：过大的Secondary Nodes值会降低运行效率；过小的Secondary Nodes值会导致射线路径和走时计算的精确度降低。因此， 尽可能选取平衡的值。
- 快速扫描法（FSM）:  开启`fsm high order`参数会导致运行时间增加7倍以上。 但是，是否开启参数对射线追踪的结果影响不大。 因此， 请谨慎开启这个参数。

- 动态最短路径法（DSPM）: 过大的`tertiary nodes`和 `src radius tertiary`会增加运行时间。但是，是否开启参数对射线追踪的结果影响不大。 因此， 请谨慎使用这个参数。

- 在二维模型的情况下， 使用**快速扫描法（FSM）**法效率最高，射线追踪结果相对较好。

 

### 7.2.三维模型测试

#### 7.2.1.测试参数：次级网格节点（secondary nodes）

##### 7.2.1.1. 初始参数设置

> - 算法：最短路径算法（SPM）,  只针对SPM有该参数。
>
> - 网格分辨率：（50, 50, 50）
>
> - 源点个数：1
>
> - 接收点个数：1
>
> - 3D层状的慢度：
>
>   ```python
>   # 设置慢度值
>   6.666666666666666444e-04  # 0-10层
>   4.000000000000000192e-04  # 10-30层
>   2.857142857142857357e-04  # 30-50层
>   ```







##### 7.2.1.2.编写脚本文件`TestSecondaryNodes.ps1`

- 功能：输入 secondary nodes 为 0到10，输出运行时间。脚本文件与二维测试的类似。只需修改下方代码: 

  ```powershell
  # 修改1：可执行文件的路径
  $executable = ".\ttcr3d"  # 使用相对路径或完整路径
  $configFile = ".\model3d.par"  # 使用相对路径或完整路径
  
  # 修改2：每条射线路径的平均运行时间
  $singleRayPathTime = $totalTime
  ```

##### 7.2.1.2. 测试结果

<img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409031030948.png" alt="image-20240903103005815" style="zoom:50%;" />

<img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409031035119.png" alt="image-20240903103501945" style="zoom:50%;" />

- 每增加一个次级节点， 运行时间以指数型增长， 不建议在三维模型使用过大的次级节点数量，例如， `secondary nodes` $\leq 1$。 但是， 如下图， 当`secondary nodes = 0`时， 初至走时剖面和射线路径的效果**远不如**`secondary nodes = 1`的效果。

- secondary nodes = 0, 

  > - 运行时间:  2.4771132 s 
  >
  > - 接收器的初至走时：0.0266846912 s
  >
  > - 初至走时剖面：
  >
  >   ![image-20240903225214521](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409032252664.png)
  >
  > - 射线路径：
  >
  >   ![image-20240903225431216](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409032254336.png)

- secondary nodes = 1,

> - 运行时间:  28.9897932 s 
>
> - 接收器的初至走时：0.0257709882 s
>
> - 初至走时剖面：
>
>   ![image-20240903225228207](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409032252351.png)
>
> - 射线路径：
>
>   ![image-20240903225443014](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409032254145.png)





#### 7.2.2.测试快速扫描法的参数：最大扫描迭代次数（max number of iteration）

> - max number of iteration = 20， 运行时间 5.8272346s
> - max number of iteration = 100， 运行时间 5.852777s

- 此结果说明：`epsilon=1e-15` 已经使计算在较早的迭代次数就收敛了。 因此，不需要变化这个参数。

  

#### 7.2.3.测试快速扫描法的参数：是否启用三阶加权本质非振荡（WENO）算子（fsm high order）

##### 7.2.3.1. 初始参数设置

> - 算法：快速扫描法（FSM）,  只针对FSM有该参数。
>
> - 网格分辨率：（50, 50, 50）
>
> - 源点个数：1
>
> - 接收点个数：1
>
> - 3D层状的慢度：
>
>   ```python
>   # 设置慢度值
>   6.666666666666666444e-04  # 0-10层
>   4.000000000000000192e-04  # 10-30层
>   2.857142857142857357e-04  # 30-50层
>   ```

##### 7.2.3.2.测试结果

- 不启用该算子：

  > - 运行时间： 5.8272346 s
  >
  > - 接收器的初至走时：0.026081996 s
  >
  > - 初至走时剖面：
  >
  >   ![image-20240903224438841](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409032244981.png)
  >
  > - 射线路径：
  >
  >   ![image-20240903224051759](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409032240949.png)
  >
  >   
  >
  >   

- 启用该算子：

  > - 运行时间: 20.5187435 s 
  >
  > - 接收器的初至走时： 0.0254141736 s
  >
  > - 初至走时剖面：
  >
  >   ![image-20240903224457991](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409032244132.png)
  >
  > - 射线路径：
  >
  >   ![image-20240903224118182](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409032241349.png)

- 此结果说明， 在三维模型下， 不启用该算子运行速度更快.  但是，两种方法接收器接收到的走时有一定差异。



#### 7.2.4.测试动态最短路径算法的参数：三级节点的个数和三级节点的半径（tertiary nodes， src radius tertiary）

##### 7.2.4.1. 初始参数设置

> - 算法：动态最短路径算法（DSPM）,  只针对DSPM有该参数。
>
> - 网格分辨率：（50, 50, 50）
>
> - 源点个数：1
>
> - 接收点个数：1
>
> - 3D层状的慢度：
>
>   ```python
>   # 设置慢度值
>   6.666666666666666444e-04  # 0-10层
>   4.000000000000000192e-04  # 10-30层
>   2.857142857142857357e-04  # 30-50层
>   ```



##### 7.2.4.2. 测试结果



![image-20240903231217835](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409032312952.png)

<img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409032313449.png" alt="image-20240903231316335" style="zoom:50%;" />

- 当三级节点数量tertiary nodes $\ge$ 7, 运行时间大大增加。但是，如下图， 初至走时剖面和射线路径的结果都较差。建议不要使用这个算法。

- tertiary nodes = 1， src radius tertiary = 1

> - 运行时间：6.0085972 s
>
> - 接收器的初至走时：0.0265546708 s ; 
>
> - 初至走时剖面：
>
>   ![image-20240903232447509](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409032324637.png)
>
> - 射线路径：
>
>   <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409032325882.png" alt="image-20240903232531741" style="zoom:50%;" />
>
>   

- tertiary nodes = 10， src radius tertiary = 10

> - 运行时间：24.6299639 s
>
> - 初至走时：0.0260289673 s
>
> - 初至走时剖面：
>
>   ![image-20240903232425099](https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409032324238.png)
>
> - 射线路径：
>
>   <img src="https://raw.githubusercontent.com/YONG916/cloudimg1/main/img5202409032325176.png" alt="image-20240903232549030" style="zoom:50%;" />



#### 7.2.5. 总结

- 在结果相对较好的参数下， 算法运行时间对比

  |                              | SPM<br />（secondary nodes = 1） | FSM<br />（fsm high order = 0） | DSPM<br />（tertiary nodes = 1， src radius tertiary = 1） |
  | ---------------------------- | -------------------------------- | ------------------------------- | ---------------------------------------------------------- |
  | Running Time<br />（Seconds) | 28.9897932                       | **5.8272346**                   | 6.0085972                                                  |

  

- 最短路径算法（SPM）：在三维模型下， secondary nodes的值为0的时候， 走时场和射线结果较差。为了平衡运行时间和算法结果， 建议secondary nodes = 1；

- 快速扫描法（FSM）:  开启`fsm high order`参数会导致运行时间增加3到4倍以上。 但是，是否开启参数对射线追踪的结果影响不大。 因此， 请谨慎开启这个参数。

- 动态最短路径法（DSPM）: 过大的`tertiary nodes`和 `src radius tertiary`会增加运行时间， 尤其是当增加三级节点`tertiary nodes`数量会显著影响运行时间。并且， 由于走时场和射线结果较差， 不建议在均匀层状模型中使用这种算法。

- 在三维模型的情况下， 使用**快速扫描法（FSM）**法效率最高，射线追踪结果相对较好。



















>>>>>>> 76a4ae9 (test ttcr)
