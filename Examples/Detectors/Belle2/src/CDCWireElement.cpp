#include "ActsExamples/Belle2/CDCWireElement.hpp"

CDCWireElement::CDCWireElement(const Acts::GeometryIdentifier& identifier,
                               const Acts::Transform3& transform,
                               std::shared_ptr<const Acts::LineBounds> bounds,
                               double thickness)
    : m_identifier(identifier), 
      m_transform(transform), 
      m_thickness(thickness) 
{
  m_surface = Acts::Surface::makeShared<Acts::StrawSurface>(bounds, *this);
}

Acts::GeometryIdentifier CDCWireElement::identifier() const {
     return m_identifier; 
}

const Acts::Transform3& CDCWireElement::localToGlobalTransform(
    const Acts::GeometryContext& /*gctx*/) const { 
    return m_transform; 
}
const Acts::Surface& CDCWireElement::surface() const {
     return *m_surface; 
}

Acts::Surface& CDCWireElement::surface() { 
    return *m_surface; 
}

double CDCWireElement::thickness() const { 
    return m_thickness; 
}

bool CDCWireElement::isSensitive() const {
    return true; 
}