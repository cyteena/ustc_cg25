
# 绘制图形

## 绘制椭圆

参考 `AddEllipse` 函数:
```c++
void ImDrawList::AddEllipse(const ImVec2& center, const ImVec2& radius, ImU32 col, float rot, int num_segments, float thickness)
```
这里的设置：
- [ ] center -> 鼠标左键第一次按下的坐标
- [ ] radius -> 鼠标现在的坐标（计算横半径和纵半径）
- [ ] rot -> 利用现在鼠标坐标与center的差值取反正弦

### rot

画一个水平的椭圆是相对容易的，如何把角度的更新考虑进去呢？**如何让用户决定何时画一个可以旋转的椭圆呢？**

添加Alt键判断

```c++
const bool is_rotating = ImGui::GetIO().KeyAlt;
```

#### 代码1

```c++
if (is_rotating)

{

	float dx = x - start_point_x_;

	float dy = y - start_point_y_;

	rotation_angle_ = atan2f(dy, dx);
}

else

{

	radius_x_ = x - start_point_x_;

	radius_y_ = y - start_point_y_;

}
```

用户自定义的问题解决了，但是旋转和更新大小分开，会导致椭圆的变化不够流畅。于是我们修改代码

#### 代码1.1

```c++
if (is_rotating)

{

float dx = x - start_point_x_;

float dy = y - start_point_y_;

rotation_angle_ = atan2f(dy, dx);

radius_x_  = dx;

radius_y_  = dy;

}

else

{

radius_x_ = x - start_point_x_;

radius_y_ = y - start_point_y_;

}
```

这样在旋转椭圆时，椭圆也可以自由变换大小

---

#### 未来的改进空间

即便旋转椭圆时的变换流畅，但是按下和松开Alt的变化依然有突变感


---

## 绘制多边形

### 如何绘制一个多边形？

![polygon.gif](https://github.com/USTC-CG/USTC_CG_25/raw/main/Homeworks/1_mini_draw/documents/figs/polygon.gif "效果示意图")

1. 逐步把边画出来，可以使用`AddLine`函数
	- 期望效果： 按下鼠标左键，添加一个顶点
		- 如何实现这个效果呢？简单，把顶点`(x, y)` `push_back()` 到坐标向量`x_list_`,`y_list_`的最后一个
		- 那如何实现在绘制多边形的过程中，移动鼠标出现的动画效果呢？我们用`update`更新最后一个点就可以 `x_list_.back() = x`

很容易写出下面符合直觉的代码

```c++
void Polygon::update(float x, float y)

{

if (!x_list_.empty())

    {

        x_list_.back() = x;

        y_list_.back() = y;

    }

}

void Polygon::add_control_point(float x, float y)

{

    x_list_.push_back(x);

    y_list_.push_back(y);

}
```

实际上，上面的代码实现连一条边都画不出来。查看过程后，发现第一次按下左键，将鼠标坐标点添加到`x_list_, y_list_`中，此时`x_list_, y_list_`只有一个点。鼠标移动调用`update`函数，`update`函数直接把先前添加的坐标点给改掉了。

ok，那很容易把代码改成下面的样子
```c++
void Polygon::update(float x, float y)

{

    if (x_list_.size() == 1)

    {

        x_list_.push_back(x);

        y_list_.push_back(y);

    }

    else if (!x_list_.empty())

    {

        x_list_.back() = x;

        y_list_.back() = y;

    }

}
```
`add_control_point`函数不变，再次运行，发现成功实现！

虽然很容易就改对了代码，但是其中成功画图的逻辑并不显然

![](https://raw.githubusercontent.com/cyteena/pic/main/20250227114455838.png)
#### 实现右键点击完成绘制

有了前面的铺垫，这里的思路非常直接，直接取出第一个点，`push_back`就可以了

为了取出第一个点坐标，此次作业在`Shape` 添加
```c++
virtual int get_control_points_count() const

{

	return 0;

};

virtual ControlPoint get_control_point(int index) const

{

	return { 0.0, 0.0 };

};
```

顺便定义

```c++
struct ControlPoint

{

    float x;

    float y;

};
```

并在`Polygon`相应实现

#### 添加限制

添加`get_control_point`，必须有三个固定点才可以执行多边形绘制完毕
#### 实现过程中遇见的问题

误将`Mouse-click-event()`中关于绘制`kPolygon`的`draw_status`设置为`fasle`

![](https://raw.githubusercontent.com/cyteena/pic/main/20250227_120711%20(online-video-cutter.com)%20(1).gif)

---

## 实现自由绘制

自由绘制相较于多边形绘制反而更简单，只用将`update`实现为`push_back`即可


# 结果展示



![](https://raw.githubusercontent.com/cyteena/pic/main/20250227_120711_result.gif)

---
# 导出类图


![](https://raw.githubusercontent.com/cyteena/pic/main/1_MiniDraw.png)

---

## 遇见的Bug

### chocolatey默认安装路径

chocolatey默认将软件安装在C盘，[据说修改默认下载路径要付费](https://juejin.cn/post/6844903782854164487)

![](https://raw.githubusercontent.com/cyteena/pic/main/20250227124750156.png)

### clang-uml运行时提示找不到头文件或头文件格式错误


此次作业使用的mingw-g++/gcc来编译，错误原因应该是clang-uml在Windows上默认情况不能处理`mingw-g++/gcc` 编译的代码。

解决方法：[参考这个issue](https://github.com/bkryza/clang-uml/issues/257)

![](https://raw.githubusercontent.com/cyteena/pic/main/20250227125305862.png)
