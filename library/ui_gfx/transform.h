
#ifndef __ui_gfx_transform_h__
#define __ui_gfx_transform_h__

#pragma once

#include "SkMatrix44.h"

namespace gfx
{

class Rect;
class Point;
class Point3f;

// 4x4 transformation matrix. Transform is cheap and explicitly allows
// copy/assign.
class Transform
{
public:
    Transform();
    ~Transform();

    bool operator==(const Transform& rhs) const;
    bool operator!=(const Transform& rhs) const;

    // NOTE: The 'Set' functions overwrite the previously set transformation
    // parameters. The 'Concat' functions apply a transformation (e.g. rotation,
    // scale, translate) on top of the existing transforms, instead of overwriting
    // them.

    // NOTE: The order of the 'Set' function calls do not matter. However, the
    // order of the 'Concat' function calls do matter, especially when combined
    // with the 'Set' functions.

    // Sets the rotation of the transformation.
    void SetRotate(float degree);

    // Sets the scaling parameters.
    void SetScaleX(float x);
    void SetScaleY(float y);
    void SetScale(float x, float y);

    // Sets the translation parameters.
    void SetTranslateX(float x);
    void SetTranslateY(float y);
    void SetTranslate(float x, float y);

    // Applies rotation on the current transformation.
    void ConcatRotate(float degree);

    // Applies scaling on current transform.
    void ConcatScale(float x, float y);

    // Applies translation on current transform.
    void ConcatTranslate(float x, float y);

    // Applies a transformation on the current transformation
    // (i.e. 'this = this * transform;').
    void PreconcatTransform(const Transform& transform);

    // Applies a transformation on the current transformation
    // (i.e. 'this = transform * this;').
    void ConcatTransform(const Transform& transform);

    // Does the transformation change anything?
    bool HasChange() const;

    // Applies the transformation on the point. Returns true if the point is
    // transformed successfully.
    void TransformPoint(gfx::Point3f& point) const;

    // Applies the transformation on the point. Returns true if the point is
    // transformed successfully. Rounds the result to the nearest point.
    void TransformPoint(gfx::Point& point) const;

    // Applies the reverse transformation on the point. Returns true if the
    // transformation can be inverted.
    bool TransformPointReverse(gfx::Point3f& point) const;

    // Applies the reverse transformation on the point. Returns true if the
    // transformation can be inverted. Rounds the result to the nearest point.
    bool TransformPointReverse(gfx::Point& point) const;

    // Applies transformation on the rectangle. Returns true if the transformed
    // rectangle was axis aligned. If it returns false, rect will be the
    // smallest axis aligned bounding box containg the transformed rect.
    void TransformRect(gfx::Rect* rect) const;

    // Applies the reverse transformation on the rectangle. Returns true if
    // the transformed rectangle was axis aligned. If it returns false,
    // rect will be the smallest axis aligned bounding box containg the
    // transformed rect.
    bool TransformRectReverse(gfx::Rect* rect) const;

    // Returns the underlying matrix.
    const SkMatrix44& matrix() const
    {
        return matrix_;
    }
    SkMatrix44& matrix()
    {
        return matrix_;
    }

private:
    void TransformPointInternal(const SkMatrix44& xform,
                                gfx::Point& point) const;

    void TransformPointInternal(const SkMatrix44& xform,
                                gfx::Point3f& point) const;

    SkMatrix44 matrix_;

    // copy/assign are allowed.
};

} //namespace gfx

#endif //__ui_gfx_transform_h__