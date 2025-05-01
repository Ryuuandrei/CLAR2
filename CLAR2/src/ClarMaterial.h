#pragma once

#include <glm/glm.hpp>
#include <memory>

namespace CLAR {
	enum MaterialType {
		LAMBERTIAN,
		METAL,
		DIELECTRIC,
		DIFFUSE_LIGHT,
	};

	struct Material {
		virtual ~Material() = default;
		virtual MaterialType GetType() const = 0;
		virtual glm::vec3 GetAlbedo() const { return glm::vec3(0.f); }
		virtual float GetFuzzOrRefractionIndex() const { return 0.f; }
	};

	struct Lambertian : public Material {
		glm::vec3 albedo;
		Lambertian(const glm::vec3& albedo) : albedo(albedo) {}
		MaterialType GetType() const override { return LAMBERTIAN; }
		glm::vec3 GetAlbedo() const override { return albedo; }
	};

	struct Metal : public Material {
		glm::vec3 albedo;
		float fuzz;
		Metal(const glm::vec3& albedo, float fuzz = 0.0f) : albedo(albedo), fuzz(fuzz) {}
		MaterialType GetType() const override { return METAL; }
		glm::vec3 GetAlbedo() const override { return albedo; }
		float GetFuzzOrRefractionIndex() const override { return fuzz; }
	};

	struct Dielectric : public Material {
		float refractionIndex;
		Dielectric(float refractionIndex = 1.0f) : refractionIndex(refractionIndex) {}
		MaterialType GetType() const override { return DIELECTRIC; }
		glm::vec3 GetAlbedo() const override { return glm::vec3(1.f); }
		float GetFuzzOrRefractionIndex() const override { return refractionIndex; }
		// Note: Albedo is not applicable for Dielectric, so we return a default value
	};

	struct DiffuseLight : public Material {
		glm::vec3 color;
		float area;
		DiffuseLight(const glm::vec3& color, float area = 1.f) : color(color), area(area) {}
		MaterialType GetType() const override { return DIFFUSE_LIGHT; }
		glm::vec3 GetAlbedo() const override { return color; }
		float GetFuzzOrRefractionIndex() const override { return area; }
	};


	class MaterialFactory {
		public:
		static std::shared_ptr<Material> CreateMaterial(MaterialType type, const glm::vec3& albedo = glm::vec3(0.f), float fuzzOrRefractionIndex = 0.f) {
			switch (type) {
			case LAMBERTIAN:
				return std::make_shared<Lambertian>(albedo);
			case METAL:
				return std::make_shared<Metal>(albedo, fuzzOrRefractionIndex);
			case DIELECTRIC:
				return std::make_shared<Dielectric>(fuzzOrRefractionIndex);
			case DIFFUSE_LIGHT:
				return std::make_shared<DiffuseLight>(albedo, fuzzOrRefractionIndex);
			default:
				throw std::invalid_argument("Unknown material type");
			}
		}
	};
}