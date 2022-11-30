#pragma once
#include <cassert>
#include "Math.h"

namespace dae
{
	namespace BRDF // code from the RayTracer
	{
		/**
		 * \param kd Diffuse Reflection Coefficient
		 * \param cd Diffuse Color
		 * \return Lambert Diffuse Color
		 */
		static ColorRGB Lambert(float kd, const ColorRGB& cd)
		{
			return ((cd * kd) * INV_PI);
		}

		static ColorRGB Lambert(const ColorRGB& kd, const ColorRGB& cd)
		{
			return ((cd * kd) * INV_PI);
		}

		/**
		 * \brief todo
		 * \param ks Specular Reflection Coefficient
		 * \param exp Phong Exponent
		 * \param l Incoming (incident) Light Direction
		 * \param v View Direction
		 * \param n Normal of the Surface
		 * \return Phong Specular Color
		 */
		static ColorRGB Phong(const ColorRGB& specularColor, float ks, float exp, const Vector3& l, const Vector3& v, const Vector3& n)
		{
			//const Vector3 reflect{ l - 2 * (Vector3::Dot(n, l)) * n };
			const Vector3 reflect{ Vector3::Reflect(l,n)};
			float angleViewReflect{ std::max(Vector3::Dot(reflect, v), 0.0f) }; // angle between the reflect and view
			// -> max out angleViewReflect to be at least 0, this will spare an if statement
			const float phongValue{ ks * powf(angleViewReflect, exp) };

			return { specularColor * ColorRGB{ phongValue, phongValue, phongValue } };
		}

	}
}