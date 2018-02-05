//�������ͶӰ��Ⱦ
struct Light
{
	float3 dir;		//���ߴ�������
	float cone;
	float3 pos;		//��Դλ��(��������)
	float  range;	//��Դ���䷶Χ
	float3 att;		//˥��ϵ��
	float4 ambient;	//������
	float4 diffuse;	//�������
};

cbuffer cbPerFrame:register(b1)
{
	Light light;
};

cbuffer cbPerObject:register(b0)
{
	float4x4 WVP;			//�۲����ģ��*�۲�*ͶӰ����
	float4x4 World;			//�������
	float4x4 lightPos;		//��Դ����
	float4x4 ProjectorView;	//��Դ����������۲����
	float4x4 ProjectorProj; //��Դ���������ͶӰ����
};

Texture2D ObjTexture:register(t0);			   //��������
Texture2D ShadowMap:register(t1);			   //ͶӰ����
SamplerState ObjSamplerState:register(s0);	   //���������ʽ
SamplerState shaderSampleType:register(s1);    //��Ӱ������ʽ

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;			//��������
	float4 worldPos : POSITION;			//��Ӧ����������
	float2 TexCoord : TEXCOORD;			//��Ӧ����������
	float3 normal : NORMAL;				//������
	float4 projPos: POSITION2;		//���ڵ��ԴͶӰ����βü��ռ������
};

VS_OUTPUT VS(float4 inPos : POSITION, float2 inTexCoord : TEXCOORD, float3 normal : NORMAL)
{
	VS_OUTPUT output;
	output.Pos = mul(inPos, WVP);			//���ص�����
	output.worldPos = inPos;				//��������
	output.normal = mul(normal, World);		//���������еķ���
	output.normal = normalize(output.normal);		//���������еķ���
	output.TexCoord = inTexCoord;			//��������

	//������任����Դ�������ͶӰ�µ���βü��ռ�
	output.projPos = inPos;
	output.projPos = mul(output.projPos, ProjectorView);
	output.projPos = mul(output.projPos, ProjectorProj);

	return output;
}

float4 PS_Shadow(VS_OUTPUT outa) : SV_Target
{

	float4 TexColor; //�ɼ�����������ɫ
	float  ShadowMapDepth; //�洢�����
	float2 ShadowTex;   //��Ӱ��������
	float3 finalColor = { 0.0f, 0.0f, 0.0f }; //�����������ɫ
	float  Depth;			//����Դ�����
	float  bias;				//�趨����ֵ

	//����ƫ����
	bias = 0.0001f;

	//��һ,��ȡ��������Ĳ�����ɫ
	TexColor = ObjTexture.Sample(ObjSamplerState, outa.TexCoord);

	//�ڶ���������û���ڵ�����Ӧ�þ߱�������,ע�⻷���ⲻ������Ӱ,����������������������Ӱ
	float3 finalAmbient = TexColor * light.ambient;


	//����,�����Ӧ���������Ӧ��ShdowMap�ϵ����ֵ
	//��ȡͶӰ����µ�ͶӰ����ռ������ֵ[0.0,1.0]  u=0.5*x+0.5;   v=-0.5*y+0.5;   -w<=x<=w  -w<=y<=w  
	ShadowTex.x = (outa.projPos.x / outa.projPos.w)*0.5f + 0.5f;
	ShadowTex.y = (outa.projPos.y / outa.projPos.w)*(-0.5f) + 0.5f;


	//����,����3Dģ�Ϳ��ܳ���ͶӰ����µ��ӽ��壬��ͶӰ������ܲ���[0.0,1.0],���Եý����ж����3D����ͶӰ�Ĳ����Ƿ����ӽ�����
	//if (saturate(ShadowTex.x) == ShadowTex.x && saturate(ShadowTex.y) == ShadowTex.y)
	if (true)
	{
		//����������������Ӧ�����ֵ
		ShadowMapDepth = ShadowMap.Sample(shaderSampleType, ShadowTex).r;
		//�������������Ӧ�����ֵ(���Դ����Ⱦ������ֵ)
		Depth = (float)outa.projPos.z / outa.projPos.w;

		//��ȥ��Ӱƫб��
		Depth = Depth - bias;

		//��������ڵ�,������߱��������
		if (ShadowMapDepth >= Depth)      //ԭ������if (ShadowMapDepth >= Depth) ��������û�����õ��Ľ��ȴ���෴��
		{
			//return float4(0, 1, 0, 1);

			//����������ĵķ���
			float3 lightToPixelVec = light.pos - outa.worldPos;
			//����۹�Ƶ����صľ���
			float distance = length(lightToPixelVec);

			float3 lightdir_identity = light.dir / length(light.dir); //���շ���λ������֤����ֵ���ܴ���1����������

			//���������ھ۹�Ƶ����䷶Χ����ô�����յ����ص���ɫ��Ϊ���������
			if (distance > light.range)
				return float4(finalAmbient, TexColor.a);

			//��Դ����ĵ�λ����
			lightToPixelVec /= distance;

			//������
			//float howMuchLight = dot(lightToPixelVec, outa.normal);

			//If light is striking the front side of the pixel
			//if (howMuchLight > 0.0f)
			//{
				//Add light to the finalColor of the pixel
				finalColor += TexColor * light.diffuse;

				//���ߵ�˥������
				finalColor /= light.att[0] + (light.att[1] * distance) + (light.att[2] * (distance*distance));

				//���������ĵ����ߵ�˥��
				finalColor *= pow(max(dot(-lightToPixelVec, lightdir_identity), 0.0f), light.cone);
			//}
		}
		//else{	//��Ӱ����
		//	return float4(finalAmbient*0.7567, TexColor.a);
		//}
	}
	//make sure the values are between 1 and 0, and add the ambient
	finalColor = saturate(finalColor + finalAmbient);

	return float4(finalColor, TexColor.a);

}

/*
* �������Ƴ���
*/
float4 PS(VS_OUTPUT input) : SV_TARGET
{
	input.normal = normalize(input.normal);		//��λ����

	float4 diffuse = ObjTexture.Sample(ObjSamplerState, input.TexCoord);	//�����ص��������ɫֵ

		float3 finalColor = float3(0.0f, 0.0f, 0.0f);	//���յ����ص���ɫ�������ʼ��Ϊ��ɫ

		//���������У������ϵ�һ����㵽��Դ�ķ�������
		float3 lightToPixelVec = light.pos - input.worldPos;

		//���������е����Դ�ľ���
		float d = length(lightToPixelVec);

	//���յ����ص���ɫֵ�еĻ��������
	float3 finalAmbient = diffuse * light.ambient;

		float3 lightdir_identity = light.dir / length(light.dir); //���շ���λ������֤����ֵ���ܴ���1����������

		//���������ڵ��Դ�����䷶Χ����ô�����յ����ص���ɫ��Ϊ���������
	if (d > light.range)
		return float4(finalAmbient, diffuse.a);

	//��Դ����ĵ�λ����
	lightToPixelVec /= d;

	//������
	float howMuchLight = dot(lightToPixelVec, input.normal);

	//If light is striking the front side of the pixel
	if (howMuchLight > 0.0f)
	{
		//Add light to the finalColor of the pixel
		finalColor += diffuse * light.diffuse;

		//���ߵ�˥������
		finalColor /= light.att[0] + (light.att[1] * d) + (light.att[2] * (d*d));

		//���������ĵ����ߵ�˥��
		finalColor *= pow(max(dot(-lightToPixelVec, lightdir_identity), 0.0f), light.cone);
	}

	//make sure the values are between 1 and 0, and add the ambient
	finalColor = saturate(finalColor + finalAmbient);

	return float4(finalColor, diffuse.a);
}

float4 D2D_PS(VS_OUTPUT input) : SV_TARGET
{
	//����������ĵķ���
	float3 lightToPixelVec = light.pos - input.worldPos;
	//����۹�Ƶ����صľ���
	/*float distance = length(lightToPixelVec);
	if (distance <= 2)
	return (1, 1, 1, 1);
	else return (0, 0, 0, 0);*/
	float4 color = ObjTexture.Sample(ObjSamplerState, input.TexCoord);	//�����ص��������ɫֵ
	return color;
}


