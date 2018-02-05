//正常相机投影渲染
struct Light
{
	float3 dir;		//光线传播方向
	float cone;
	float3 pos;		//光源位置(世界坐标)
	float  range;	//光源照射范围
	float3 att;		//衰减系数
	float4 ambient;	//环境光
	float4 diffuse;	//漫反射光
};

cbuffer cbPerFrame:register(b1)
{
	Light light;
};

cbuffer cbPerObject:register(b0)
{
	float4x4 WVP;			//观察相机模型*观察*投影矩阵
	float4x4 World;			//相机坐标
	float4x4 lightPos;		//光源坐标
	float4x4 ProjectorView;	//光源处辅助相机观察矩阵
	float4x4 ProjectorProj; //光源处辅助相机投影矩阵
};

Texture2D ObjTexture:register(t0);			   //物体纹理
Texture2D ShadowMap:register(t1);			   //投影纹理
SamplerState ObjSamplerState:register(s0);	   //纹理采样方式
SamplerState shaderSampleType:register(s1);    //阴影采样方式

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;			//像素坐标
	float4 worldPos : POSITION;			//对应的世界坐标
	float2 TexCoord : TEXCOORD;			//对应的纹理坐标
	float3 normal : NORMAL;				//法向量
	float4 projPos: POSITION2;		//基于点光源投影在齐次裁剪空间的坐标
};

VS_OUTPUT VS(float4 inPos : POSITION, float2 inTexCoord : TEXCOORD, float3 normal : NORMAL)
{
	VS_OUTPUT output;
	output.Pos = mul(inPos, WVP);			//像素点坐标
	output.worldPos = inPos;				//世界坐标
	output.normal = mul(normal, World);		//世界坐标中的法线
	output.normal = normalize(output.normal);		//世界坐标中的法线
	output.TexCoord = inTexCoord;			//纹理坐标

	//将坐标变换到光源辅助相机投影下的齐次裁剪空间
	output.projPos = inPos;
	output.projPos = mul(output.projPos, ProjectorView);
	output.projPos = mul(output.projPos, ProjectorProj);

	return output;
}

float4 PS_Shadow(VS_OUTPUT outa) : SV_Target
{

	float4 TexColor; //采集基础纹理颜色
	float  ShadowMapDepth; //存储的深度
	float2 ShadowTex;   //阴影纹理坐标
	float3 finalColor = { 0.0f, 0.0f, 0.0f }; //最终输出的颜色
	float  Depth;			//点距光源的深度
	float  bias;				//设定的阈值

	//设置偏移量
	bias = 0.0001f;

	//第一,获取基础纹理的采样颜色
	TexColor = ObjTexture.Sample(ObjSamplerState, outa.TexCoord);

	//第二，不管有没有遮挡，都应该具备环境光,注意环境光不生成阴影,这里仅仅是漫反射光生成阴影
	float3 finalAmbient = TexColor * light.ambient;


	//第三,求出相应顶点坐标对应在ShdowMap上的深度值
	//获取投影相机下的投影纹理空间的坐标值[0.0,1.0]  u=0.5*x+0.5;   v=-0.5*y+0.5;   -w<=x<=w  -w<=y<=w  
	ShadowTex.x = (outa.projPos.x / outa.projPos.w)*0.5f + 0.5f;
	ShadowTex.y = (outa.projPos.y / outa.projPos.w)*(-0.5f) + 0.5f;


	//第四,由于3D模型可能超出投影相机下的视截体，其投影纹理可能不在[0.0,1.0],所以得进行判定这个3D物体投影的部分是否在视截体内
	//if (saturate(ShadowTex.x) == ShadowTex.x && saturate(ShadowTex.y) == ShadowTex.y)
	if (true)
	{
		//求出顶点纹理坐标对应的深度值
		ShadowMapDepth = ShadowMap.Sample(shaderSampleType, ShadowTex).r;
		//求出顶点坐标相应的深度值(点光源到渲染点的深度值)
		Depth = (float)outa.projPos.z / outa.projPos.w;

		//减去阴影偏斜量
		Depth = Depth - bias;

		//如果不被遮挡,则物体具备漫反射光
		if (ShadowMapDepth >= Depth)      //原来的是if (ShadowMapDepth >= Depth) ，理论上没错，但得到的结果却是相反的
		{
			//return float4(0, 1, 0, 1);

			//求出漫反射光的的方向
			float3 lightToPixelVec = light.pos - outa.worldPos;
			//求出聚光灯到像素的距离
			float distance = length(lightToPixelVec);

			float3 lightdir_identity = light.dir / length(light.dir); //光照方向单位化，保证各项值不能大于1，否则会过曝

			//如果距离大于聚光灯的照射范围，那么，最终的像素点颜色即为环境光分量
			if (distance > light.range)
				return float4(finalAmbient, TexColor.a);

			//光源到点的单位向量
			lightToPixelVec /= distance;

			//光亮度
			//float howMuchLight = dot(lightToPixelVec, outa.normal);

			//If light is striking the front side of the pixel
			//if (howMuchLight > 0.0f)
			//{
				//Add light to the finalColor of the pixel
				finalColor += TexColor * light.diffuse;

				//光线的衰减因子
				finalColor /= light.att[0] + (light.att[1] * distance) + (light.att[2] * (distance*distance));

				//计算光从中心到两边的衰减
				finalColor *= pow(max(dot(-lightToPixelVec, lightdir_identity), 0.0f), light.cone);
			//}
		}
		//else{	//阴影区域
		//	return float4(finalAmbient*0.7567, TexColor.a);
		//}
	}
	//make sure the values are between 1 and 0, and add the ambient
	finalColor = saturate(finalColor + finalAmbient);

	return float4(finalColor, TexColor.a);

}

/*
* 正常绘制场景
*/
float4 PS(VS_OUTPUT input) : SV_TARGET
{
	input.normal = normalize(input.normal);		//单位法线

	float4 diffuse = ObjTexture.Sample(ObjSamplerState, input.TexCoord);	//该像素点的纹理颜色值

		float3 finalColor = float3(0.0f, 0.0f, 0.0f);	//最终的像素点颜色，这里初始化为黑色

		//世界坐标中，像素上的一坐标点到光源的方向向量
		float3 lightToPixelVec = light.pos - input.worldPos;

		//世界坐标中点与光源的距离
		float d = length(lightToPixelVec);

	//最终的像素点颜色值中的环境光分量
	float3 finalAmbient = diffuse * light.ambient;

		float3 lightdir_identity = light.dir / length(light.dir); //光照方向单位化，保证各项值不能大于1，否则会过曝

		//如果距离大于点光源的照射范围，那么，最终的像素点颜色即为环境光分量
	if (d > light.range)
		return float4(finalAmbient, diffuse.a);

	//光源到点的单位向量
	lightToPixelVec /= d;

	//光亮度
	float howMuchLight = dot(lightToPixelVec, input.normal);

	//If light is striking the front side of the pixel
	if (howMuchLight > 0.0f)
	{
		//Add light to the finalColor of the pixel
		finalColor += diffuse * light.diffuse;

		//光线的衰减因子
		finalColor /= light.att[0] + (light.att[1] * d) + (light.att[2] * (d*d));

		//计算光从中心到两边的衰减
		finalColor *= pow(max(dot(-lightToPixelVec, lightdir_identity), 0.0f), light.cone);
	}

	//make sure the values are between 1 and 0, and add the ambient
	finalColor = saturate(finalColor + finalAmbient);

	return float4(finalColor, diffuse.a);
}

float4 D2D_PS(VS_OUTPUT input) : SV_TARGET
{
	//求出漫反射光的的方向
	float3 lightToPixelVec = light.pos - input.worldPos;
	//求出聚光灯到像素的距离
	/*float distance = length(lightToPixelVec);
	if (distance <= 2)
	return (1, 1, 1, 1);
	else return (0, 0, 0, 0);*/
	float4 color = ObjTexture.Sample(ObjSamplerState, input.TexCoord);	//该像素点的纹理颜色值
	return color;
}


