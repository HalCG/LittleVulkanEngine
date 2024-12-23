#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

//当顶点着色器执行完后，OpenGL会自动在顶点之间插值这些输出变量。
//由于OpenGL通常使用线性插值，fragNormalWorld变量将根据顶点的权重（即各顶点在当前片段中的比重）进行插值。
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projectionViewMatrix;
  vec4 ambientLightColor;	// 环境光，w 分量表示光强
  vec3 lightPosition;
  vec4 lightColor;			//点光源，w 分量表示光强
} ubo;

layout(push_constant) uniform PushConstantData{//每个着色器入口点只能使用一个常量推送块
	mat4 modelMatrix;
	mat4 normalMatrix;
}pushConstantData;

void main(){
		vec4 positionWorld = pushConstantData.modelMatrix * vec4(position, 1.0);
		gl_Position = ubo.projectionViewMatrix * positionWorld;


		fragNormalWorld = normalize(mat3(pushConstantData.normalMatrix) * normal);
		fragPosWorld = positionWorld.xyz;
		fragColor = color;
	}

/*
在OpenGL渲染管线中，顶点着色器和片段着色器之间的数据传递通常是通过插值实现的。在你提供的代码片段中，法向量是如何从顶点着色器传递到片段着色器的，主要是通过**varying**变量（或在现代OpenGL中称为**out**变量）来完成的。

### 插值过程

1. **变量声明**：
   在顶点着色器中，首先需要声明一个输出变量（例如`fragNormalWorld`），它将用作与片段着色器之间的连接。这通常会使用 `out` 关键字：

   ```glsl
   // 顶点着色器
   out vec3 fragNormalWorld;
   ```

2. **法向量计算**：
   在顶点着色器中，你计算法向量的世界坐标，并将其赋值给这个输出变量。比如，你的代码中有以下行：

   ```glsl
   fragNormalWorld = normalize(mat3(pushConstantData.normalMatrix) * normal);
   ```

   确保你已经将 `normal` 的数据传递给顶点着色器，并结合模型的法向量变换来获取世界空间中的法向量。

3. **默认插值**：
   当顶点着色器执行完后，OpenGL会自动在顶点之间插值这些输出变量。在片段着色器中，你需要声明与顶点着色器相对应的输入变量（例如`fragNormalWorld`），使用 `in` 关键字：

   ```glsl
   // 片段着色器
   in vec3 fragNormalWorld;
   ```

4. **使用插值后的法向量**：
   在片段着色器中，接收到的 `fragNormalWorld` 就是通过图形管线中插值操作得到的。由于OpenGL通常使用线性插值，`fragNormalWorld`变量将根据顶点的权重（即各顶点在当前片段中的比重）进行插值。

### 代码示例

下面是一个简单的示例来展示如何在顶点着色器和片段着色器中实现这种插值：

```glsl
// 顶点着色器
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;

out vec3 fragNormalWorld;
out vec3 fragPosWorld;
out vec4 fragColor;

uniform mat4 modelMatrix;
uniform mat4 projectionViewMatrix;
uniform mat4 normalMatrix;

void main() {
    vec4 positionWorld = modelMatrix * vec4(position, 1.0);
    gl_Position = projectionViewMatrix * positionWorld;

    fragNormalWorld = normalize(mat3(normalMatrix) * normal);
    fragPosWorld = positionWorld.xyz;
    fragColor = color;
}

// 片段着色器
#version 330 core

in vec3 fragNormalWorld;
in vec3 fragPosWorld;
in vec4 fragColor;

out vec4 color;

void main() {
    // 使用插值后的法线进行光照计算
    // ...
    color = fragColor; // 或者进行其他操作
}
```

### 总结

综上所述，通过在顶点着色器中定义输出变量并在片段着色器中定义对应的输入变量，OpenGL会自动进行插值处理，使得法向量（或其他属性）能够在顶点着色器与片段着色器之间顺利传递。希望这个解释对你理解插值过程有所帮助！如果你还有其他问题或需要更深入的细节，欢迎继续讨论。

*/