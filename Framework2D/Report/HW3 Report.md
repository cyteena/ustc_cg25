
HW3 要实现的效果非常直观，代码流程也不复杂，最关键最难的地方在于如何实现`build_poisson_equation`

首先我们要明确一些概念什么是`src`, `tar`, `mask`

- `src` and `mask` come from the same picture, e.g. `bear`
- `tar` is the result picture, e.g. `sea`

# index_map

由于涉及到把图像拉成一维向量，这个映射可以随便定，但重点是我们需要知道这个`map`

![](https://raw.githubusercontent.com/cyteena/pic/main/20250312121230268.png)

我们有下面的代码

![](https://raw.githubusercontent.com/cyteena/pic/main/20250312121634646.png)

# How to build the poisson equation

让我们先来回顾一下原论文的说法

>As it is enough to solve the interpolation problem for each color component separately, we consider only scalar image functions.

![](https://raw.githubusercontent.com/cyteena/pic/main/20250312123420683.png)


本质上是一个插值的工作，$f^\star$ is the known scalar function defined over $S$ minus the interior of $\Omega$, $f$ is the function we wanna to know, i.e. the unknown scalar function defined over the interior of $\Omega$. Of course, we should satisfy the boundary condition. Except that, we know the intepolation should follow some guidance, the guidance we use here is the **vector field** $\mathbf{v}$ defined over $\Omega$ 

## Minimal equation

$$\min_{f}\iint_{\Omega}|\nabla f - \mathbf{v}|^2 \quad \text{with } f|_{\partial \Omega} = f^\star|_{\partial \Omega}$$

The minimizer above is the same as the solution of the Poisson equation with Dirichlet boundary conditions:

$$
\Delta f = \text{div} \mathbf{v} \text{ over } \Omega, \text{ with } f|_{\partial \Omega} = f^*|_{\partial \Omega},
$$
if $\mathbf{v}$ is conservative, i.e. it's the gradient of some function $g$, then we can consider the 

$$
\Delta \tilde{f} = 0 \text{ over } \Omega, \ \tilde{f}|_{\partial \Omega} = (f^* - g)|_{\partial \Omega}.
$$

## Discrete Poisson solver

- For each pixel $p$ in $S$, let $N_{p}$ be the set of its 4-connected neighbors which are in $S$.
- $\braket{p,q}$ denote a pixel pair such that $q \in N_{p}$
- $f_{p}$ denote the value of $f$ at $p$
- The boundary of $\Omega$ is now $\partial \Omega = \{ p \in S \setminus \Omega : N_p \cap \Omega \neq \emptyset \}.$

### Discrete Laplacian 

**Δf(i, j) ≈ f(i+1, j) + f(i-1, j) + f(i, j+1) + f(i, j-1) - 4f(i, j)**

### Discrete Divergence of the Vector Field

![](https://raw.githubusercontent.com/cyteena/pic/main/20250312132011475.png)

Just like the _flow_

**But the picture above is Wrong!**

We should remember the divergence of a vector field means the _outgoingness_

That is the *outflow - inflow*

We should consider each direction individually,

_outflow_x - inflow_x = g(x+1, y) - g(x,y) - (g(x,y) - g(x-1, y))

Then we have _outflow - inflow = g(x+1,y)+g(x,y+1)+g(x-1,y)+g(x,y-1) - 4g(x,y)

Then $-\nabla \cdot \nabla g_{p} = \sum_{q\in N_{p}}g_{p} - g_{q}$

![](https://raw.githubusercontent.com/cyteena/pic/main/20250312135610367.png)

有点违反直觉

### Now we get the Discrete Poisson Equation

![](https://raw.githubusercontent.com/cyteena/pic/main/20250312135740605.png)

$v_{pq} = g_{p} - g_{q}$

$\sum_{q\in N_{P}}v_{pq} = |N_{p}|g_{p} - \sum_{q\in N_{p}}g_{q}$


## Turn to the Code

We succeed!

![](https://raw.githubusercontent.com/cyteena/pic/main/20250312141751859.png)

有了上面的铺垫，加上Deepseek/Gemini/Claude，写出正确的代码是自然的事情😄

# Now Let's consider `Mixing gradients`

![](https://raw.githubusercontent.com/cyteena/pic/main/20250312142554246.png)

![](https://raw.githubusercontent.com/cyteena/pic/main/20250312192257146.png)

Mixing gradient is easy to implement on the base code

# Now let's consider `Freehand` selected_region

`Freehand` 最重要的实现就是如何得到内部点 `get_interior_pixels`

我们看图就知道算法是如何写的

![](https://raw.githubusercontent.com/cyteena/pic/main/20250312232220340.png)

找到左右交点`intersections`的横坐标`x_left and x_right`，就可以把所有中间点存到`interior_pixels`当中

![](https://raw.githubusercontent.com/cyteena/pic/main/20250312232428907.png)

对于右边的交点也是一样

![](https://raw.githubusercontent.com/cyteena/pic/main/20250312232511046.png)

现在只差最后一步了，这一步非常关键，我们要考虑交点不止有两个情况
![](https://raw.githubusercontent.com/cyteena/pic/main/20250312232613806.png)

于是我们需要

![](https://raw.githubusercontent.com/cyteena/pic/main/20250312232644023.png)


# Tips

此次作业增加了`logger`打印日志功能，虽然写代码的时候要多些`logger`还挺麻烦的，但是`logger`打印出来各个通道求解的`pixel`的最小值和最大值，对于debug起到了关键的作用


# 结果展示

链接：[谌奕同_Poisson_editing_ustc_cg](https://rec.ustc.edu.cn/share/b179e190-ff58-11ef-bc1b-6f6662126c54)
密码：2jy6


