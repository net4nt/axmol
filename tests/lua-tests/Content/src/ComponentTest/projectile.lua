
local projectile = {

    sceneLua = nil,

    onEnter = function(self)
        local sceneScriptComponent = tolua.cast(self:getOwner():getParent():getComponent("sceneLuaComponent"), "ax.ComponentLua")
        self.sceneLua  = sceneScriptComponent:getScriptObject()
    end,

    update = function(self, dt)
        -- if there is any enemy collides with this projectile, then
        -- remove this projectile and all collided enemies

        local owner = self:getOwner()
        local projectileX, projectileY = owner:getPosition()
        local projectileContentSize = owner:getContentSize()
        local projectileRect = ax.rect(projectileX, projectileY,
                projectileContentSize.width, projectileContentSize.height)

        local scene = owner:getParent()
        local enemies = self.sceneLua.enemies
        --local enemies = self:getEnemies()
        local removeOwner = false

        for i = #enemies, 1, -1 do
            local enemy = enemies[i]
            local enemyX, enemyY = enemy:getPosition()
            local enemyContentSize = enemy:getContentSize()
            local enemyRect = ax.rect(enemyX, enemyY,
                    enemyContentSize.width, enemyContentSize.height)
            if ax.rectIntersectsRect(projectileRect, enemyRect) then
                table.remove(enemies, i)
                scene:removeChild(enemy, true)
                self.sceneLua:inscreaseCount()
                removeOwner = true
            end
        end

        if removeOwner == true then
            scene:removeChild(owner, true)
        end
    end,
}

return projectile
