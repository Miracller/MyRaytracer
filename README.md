
# MyRaytracer

此项目主要是使用C++开发语言实现一个基于蒙特卡洛积分法的离线光线追踪渲染器，渲染器支持多种材质和场景，支持球体和立方体的旋转位移。

- 使用多线程技术和BVH技术加速
- 引入蒙特卡洛积分法求解漫反射项，使用对光源采样和重要性采样法提高路径追踪效率。
- 引入微表面模型、perlin noise、image texture等材质。

## Result

![image](https://user-images.githubusercontent.com/24697586/229330879-1e67a2c9-0b3f-4fac-97a2-831592185400.png)
It really takes a long time to render this picture.

no on-light sampling VS on-light sampling:

![image](https://user-images.githubusercontent.com/24697586/229330985-a6845748-4723-4dc8-857c-84849edfa74a.png)

![image](https://user-images.githubusercontent.com/24697586/229331005-c7acf883-663f-4d15-8a7b-a22862e10510.png)


## 一些技术细节

1. ## BVH 基于物体划分的空间加速
    基本思想是：根据规则，将物体分为两堆，再分别求bounding box，一个物体只出现在一个盒子里。
    ### 算法：
    - 找到包围盒
    - 递归得将包围盒内物体分为两部分
    - 重新计算每个部分的包围盒
    - 达到阈值时停止划分
    - 将物体存储在叶子节点上

    ### 划分规则：
    - 总选物体分布最长的轴划分
    - 在物体顺序中的中位数处划分
    - ...


2. ## 微表面模型

    ### Cook-Torrance BRDF

    ![image](https://user-images.githubusercontent.com/24697586/229331185-4fcca77b-23e7-439e-84ec-b549970bf42c.png)

    kd指折射光占入射光比例

    ks指反射光占入射光比例

    ### 漫反射BRDF：

    f_lambert 漫反射BRDF的计算参考Games101 半球积分， $f_{lambert} = ρ / pi$

    ### 镜面反射项BRDF：

    $f_r (ωi, ωo) = F * D * G / (4 * cosθi * cosθo)$

    Where:

    - **f_r (ωi, ωo)** is the measured reflectance for incoming light direction **ωi** and outgoing light direction **ωo**
    - **F** is the Fresnel term 菲涅尔系数，根据不同的入射方向描述不同程度的反射。光与表面平行，大量能量被反射。
    - **D** is the microfacet distribution term 法线分布函数，用半程向量和平面整体（看做镜子）的法向的相近程度描述。
    - **G** is the geometric attenuation term 几何函数，描述微表面的自遮挡现象。
    - **θi** is the angle between the surface normal and incoming light direction
    - **θo** is the angle between the surface normal and outgoing light direction

    note:

    ![image](https://user-images.githubusercontent.com/24697586/229331197-e61cc04e-233e-42f8-b7fd-fd27273c96f6.png)


    1. 法线分布函数 D（Normal Distribution Fuction），NDF

    （只有与法线n方向相同的半程向量h才能将入射光正确的反射到出射方向）

    - Blinn-Phong 分布
    - Beckman 分布
    - Trowbridge-Reitz（GGX） 分布

    2. 阴影遮挡函数 G

        G1——微平面在单个方向（光照方向or视线方向）上可见比例，光照对应**遮蔽函数 masking function**；视线对应**阴影函数 shadowing function.**

        G2——微平面在光照和视线方向共同可见的比例，称为**联合遮蔽阴影函数 joint masking-shadowing function.**

        其中，G2由G1推导而来，同时一般微表面材质计算所说的几何函数就是指G2
        
        对于G函数来说，值域一般是[0,1]（这是因为法线分布函数用于描述微表面的法线分布密度，其值表示了在特定方向上法线的概率密度。因为概率密度函数的值通常是非负的，所以法线分布函数的值域也是非负的。）,
        自变量为半程向量和法线方向的点积。

        - Smith 遮蔽函数： $G(o, i) = G_1(o) * G_1(i)$

            Disney实现方法：引用GGX的规定，加入G1函数求解遮挡函数。

        - SchlickGGX - UE4：UE4采用的方法是用Schlick近似Smith来计算几何函数

3. ## 重要性采样
    蒙特卡洛积分是对渲染方程求积分的方法。（对whitted-style ray tracing 中的遇到diffuse材质停止弹射进行优化）。初学阶段都是默认pdf为uniform分布，这样的计算方式对于大部分场景是低效的。
    为了达到最好最快的渲染效果，蒙特卡洛积分中的pdf项对于不同形状的函数有不同的采样方法。
    这里引入cosine-weighted半球采样：
    ![image](https://user-images.githubusercontent.com/24697586/229331602-963c832c-d958-444c-a637-c4a04058176a.png)
    
    对于上述公式，我们为了让采样函数更好地拟合渲染方程，可将p(w)正比于cos函数。
    
    并通过pdf函数积分的半球积分为1求得，c = 1/pi
    
    ![image](https://user-images.githubusercontent.com/24697586/229331991-10e7e8bd-a12f-40e3-9093-c8eb60b8c8d8.png)
    
    进而得到
    
    ![image](https://user-images.githubusercontent.com/24697586/229332025-5c6a73d2-158a-4955-8219-0dcde904b720.png)
    
    再根据边缘概率密度和条件概率密度，及分布函数，将极坐标转化为三维坐标，进而在程序中使用。

    
