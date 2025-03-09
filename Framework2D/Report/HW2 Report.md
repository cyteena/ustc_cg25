# 展示类图

![](https://raw.githubusercontent.com/cyteena/pic/main/20250309225252581.png)

# 期望效果

- 用户通过鼠标点击操作实现图片的warp
- 用户鼠标点击分为两组点，起点和终点
- warp操作后，起点处的像素值被映射到终点处的像素值
- 其他点位得到平滑的处理
# 如何实现?

## 问题描述

给定 $n$ 对控制点 $(\boldsymbol{p}_i, \boldsymbol{q}_i)$ ，其中 $\boldsymbol{p}_i,\boldsymbol{q}_i\in\mathbb{R}^2$ ， $i=1, 2, \cdots,n$ ， 

希望得到一个函数 $f : \mathbb{R}^2\to\mathbb{R}^2$ ，满足插值条件：
$$

f(\boldsymbol{p}_i) = \boldsymbol{q}_i, \quad \text{for } i = 1, 2, \cdots, n.

$$
# Method 1: Inverse Distance-weighted Interpolation

![](https://raw.githubusercontent.com/cyteena/pic/main/20250305225626377.png)


代码实现假设线性映射$D_{i}$是恒等变换
$f(p) = \sum_{i} w_{i}(p)f_{i}(p) = \sum_{i}w_{i}(p)(q_{i} + p - p_{i}) = \sum_{i}w_{i}(p)(q_{i} - p_{i}) + p$
在实践中，我们发现这与简单的认为$D_{i} = 0$所实现的效果差异很大。

即修改成如下代码：
![](https://raw.githubusercontent.com/cyteena/pic/main/20250305230545724.png)


### 结果对比

![](https://raw.githubusercontent.com/cyteena/pic/main/20250305230709191.png)

$D_{i} = 0$ 的效果

![](https://raw.githubusercontent.com/cyteena/pic/main/20250305230725921.png)


![](https://raw.githubusercontent.com/cyteena/pic/main/20250305232100354.png)

设置$D_{i} = I$
![](https://raw.githubusercontent.com/cyteena/pic/main/20250305232113562.png)

brilliant result!

## Method 2: Radial Basis Functions Interpolation

这里选择的径向基函数是$r^2 \log r$

我们需要进行一些数学变换方便我们求解
![](https://raw.githubusercontent.com/cyteena/pic/main/20250305234403429.png)


![](https://raw.githubusercontent.com/cyteena/pic/main/20250306000436469.png)

类似可以对$y$分量写出方程

我们在这里实现了文档中提供的约束

![](https://raw.githubusercontent.com/cyteena/pic/main/20250306000642173.png)

代码实现上，我们将右边的向量替换为终点和起点的位移量（为了数值稳定）

![](https://raw.githubusercontent.com/cyteena/pic/main/20250306000854825.png)

我们使用QR分解求解上面的矩阵方程

![](https://raw.githubusercontent.com/cyteena/pic/main/20250306001319597.png)


### 结果展示

![](https://raw.githubusercontent.com/cyteena/pic/main/20250306001226137.png)

![](https://raw.githubusercontent.com/cyteena/pic/main/20250306001253886.png)

## 白缝填补

![](https://raw.githubusercontent.com/USTC-CG/USTC_CG_25/main/Homeworks/2_image_warping/documents/figs/white_stitch.jpg)


### 为什么会产生缝隙呢？

第一反应是浮点数转化为整数，导致有一些像素点未被赋予非0值。检查代码`WarpingWidget::fisheye_warping`的输出
![](https://raw.githubusercontent.com/cyteena/pic/main/20250306002216766.png)
在输出结果前，将浮点数转化为了整数。起初，图像出现缝隙有一点反直觉，因为我们的映射是连续函数，非跳跃函数，直观上不该有裂缝。

考虑到连续函数作用在离散格点上，事情就变得合理。

在近似格点过程中，不能保证单射，这就导致了不是满射，出现缝隙。

### 如何解决？

如何解决这个问题呢？一个直接的思路是，在像素值为0的像素点附近做最近邻搜索，将最近的非零值像素设置为该点的像素。

这个思路很简单，但还不够优雅。我们看看`deepseek`怎么说？`deepseek`强烈建议我们使用`反向映射`这一方法。

起初我以为`deepseek`完全理解错了我的意思，随着聊天的深入，`deepseek`完全不怀疑自己`反向映射`方法有问题，这使得我认真考虑这个方法。

![](https://raw.githubusercontent.com/cyteena/pic/main/20250306004438380.png)

具体的插值方法我们实现了三种：
- `bilinear_interpolation`
- `nearest_interpolation`
- `add_nearest_interpolation` : 使用`Annoy`第三方库
![](https://raw.githubusercontent.com/cyteena/pic/main/20250306004615299.png)
实际效果区别不大。

## Dlib

这种学一个函数的任务，怎么能少了我们的Neural Network？

我们的网络非常简单

![](https://raw.githubusercontent.com/cyteena/pic/main/20250306004952215.png)

激活函数选的`elu`，没什么特别原因，单纯看`relu`太简单，不想用。

### 这就开始训练了吗？

直接把`start_points`， `end_points` 喂给网络，就开始学习了吗？

Nono, 事情没有这么简单，如果是这么单纯的实现，你会发现网络的loss甚至会到惊人的一万+！而且即便经过一万个epoches，你的loss也很难有什么变化，这就很糟糕。

### 那我们怎么办呢？

归一化！没错，就是几乎可以出现在任何网络的任何地方的`norm`层。我们先对数据点进行归一化

![](https://raw.githubusercontent.com/cyteena/pic/main/20250306005853834.png)

![](https://raw.githubusercontent.com/cyteena/pic/main/20250306005907936.png)

同样的，推理的时候也用归一化

![](https://raw.githubusercontent.com/cyteena/pic/main/20250306005955571.png)

这样训练后，我们就可以拿到漂亮的结果。

# 结果展示


![](https://raw.githubusercontent.com/cyteena/pic/main/ezgif-310e5ccba7f0ae.gif)

## 人像编辑

![](https://raw.githubusercontent.com/cyteena/pic/main/ezgif-27fa0d27f14c28.gif)

## 完整展示视频

可以从这里查看[视频](https://rec.ustc.edu.cn/share/7a351ab0-fa59-11ef-a879-0b39fc0fa0fb) 
密码：bs4g



# 作业中遇到的bug

## 导入Dlib

由于`Dlib`包很大，而这里只需要训练一个`mlp`层，所以自然不想要把全部的`Dlib`导入进来。

折腾了半天，发现`Dlib`层层依赖的关系还挺麻烦，索性导入了整个包。

# 导出类图

感恩`cline`, 感恩`gemini-flash-thinking`
