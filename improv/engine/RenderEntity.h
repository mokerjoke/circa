
// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#pragma once

struct RenderTarget;

struct RenderEntity
{
    virtual ~RenderEntity() {}
    virtual void destroy() = 0;
    virtual bool destroyed() = 0;
};
