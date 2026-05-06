#pragma once
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Surfaces/LineBounds.hpp"
#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Surfaces/SurfacePlacementBase.hpp"

class CDCWireElement : public Acts::SurfacePlacementBase {
public:
  CDCWireElement(const Acts::GeometryIdentifier& identifier,
                 const Acts::Transform3& transform,
                 std::shared_ptr<const Acts::LineBounds> bounds,
                 double thickness = 0.);

  Acts::GeometryIdentifier identifier() const;
  const Acts::Transform3& localToGlobalTransform(const Acts::GeometryContext& gctx) const override;
  const Acts::Surface& surface() const override;
  Acts::Surface& surface() override;
  bool isSensitive() const override;
  double thickness() const ;
  std::shared_ptr<Acts::Surface> surfacePointer() const { return m_surface; }

private:
  Acts::GeometryIdentifier m_identifier;
  Acts::Transform3 m_transform;
  double m_thickness;
  std::shared_ptr<Acts::Surface> m_surface{nullptr};
};