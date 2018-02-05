//灯光相机投影渲染，生成阴影贴图
cbuffer cbLightPerFrame:register(b0)
{
	float4x4 LIGHT_WVP;
};

struct VertexIn
{
	float3 Pos:POSITION;
	float4 color:COLOR;
};


struct VertexOut
{
	float4 Pos:SV_POSITION;
	float4 color:COLOR;
};


VertexOut VS(VertexIn ina)
{
	VertexOut outa;
	outa.Pos = mul(float4(ina.Pos,1.0f), LIGHT_WVP);
	outa.color = ina.color;
	return outa;
}

float4 PS(VertexOut outa) : SV_Target
{
	return outa.color;
}