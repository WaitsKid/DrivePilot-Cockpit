"use strict";

let bridge = null;
let map = null;
let autoComplete = null;
let driving = null;
let trafficLayer = null;
let startMarker = null;
let endMarker = null;
let vehicleMarker = null;
let vehicleElement = null;
let vehicleArrowElement = null;
let vehicleSpeedElement = null;
let routeOutline = null;
let routePolyline = null;
let currentRoutePath = [];
let followingVehicle = false;
let mapReportedReady = false;
let pendingVehicleFrame = 0;
let pendingPanFrame = 0;
let pendingPanX = 0;
let pendingPanY = 0;
let userGestureActive = false;
let manualViewportActive = false;
let edgeFollowArmed = true;
let lastCenterFollowAt = 0;
let endpointFocusSignature = "";
const pendingCommands = [];
let vehicleState = {
    longitude: 120.15515,
    latitude: 30.27415,
    heading: 0,
    speed: 0,
    navigating: false
};
let endpointState = {
    hasStart: false,
    startLongitude: 0,
    startLatitude: 0,
    hasEnd: false,
    endLongitude: 0,
    endLatitude: 0
};
const NAVIGATION_ZOOM = 17;
const CENTER_FOLLOW_INTERVAL_MS = 45;
const SAFE_EDGE_MARGIN_X = 120;
const SAFE_EDGE_MARGIN_TOP = 120;
const SAFE_EDGE_MARGIN_BOTTOM = 135;
const SAFE_EDGE_INSET = 52;

const statusElement = document.getElementById("web-status");

function setStatus(text, isError = false, hide = false) {
    statusElement.textContent = text;
    statusElement.classList.toggle("error", isError);
    statusElement.classList.toggle("hidden", hide);
}

function asText(value) {
    if (Array.isArray(value))
        return value.filter(Boolean).join("、");
    if (value === null || value === undefined)
        return "";
    return String(value);
}

function asNumber(value, fallback = 0) {
    const number = Number(value);
    return Number.isFinite(number) ? number : fallback;
}

function escapeHtml(value) {
    return asText(value)
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#039;");
}

function validCoordinate(longitude, latitude) {
    return Number.isFinite(longitude) && Number.isFinite(latitude)
        && longitude >= -180 && longitude <= 180
        && latitude >= -85 && latitude <= 85;
}

function coordinateOf(location) {
    if (!location)
        return null;
    if (typeof location.getLng === "function" && typeof location.getLat === "function")
        return [location.getLng(), location.getLat()];
    if (Array.isArray(location) && location.length >= 2)
        return [asNumber(location[0], NaN), asNumber(location[1], NaN)];
    if (typeof location === "string") {
        const parts = location.split(",");
        if (parts.length >= 2)
            return [asNumber(parts[0], NaN), asNumber(parts[1], NaN)];
    }
    if (location.lng !== undefined && location.lat !== undefined)
        return [asNumber(location.lng, NaN), asNumber(location.lat, NaN)];
    return null;
}

function distanceMeters(longitude1, latitude1, longitude2, latitude2) {
    const radius = 6371008.8;
    const toRadians = value => value * Math.PI / 180;
    const latitudeA = toRadians(latitude1);
    const latitudeB = toRadians(latitude2);
    const deltaLatitude = latitudeB - latitudeA;
    const deltaLongitude = toRadians(longitude2 - longitude1);
    const sineLatitude = Math.sin(deltaLatitude / 2);
    const sineLongitude = Math.sin(deltaLongitude / 2);
    const a = sineLatitude * sineLatitude
        + Math.cos(latitudeA) * Math.cos(latitudeB) * sineLongitude * sineLongitude;
    return 2 * radius * Math.atan2(Math.sqrt(a), Math.sqrt(Math.max(0, 1 - a)));
}

function relevanceScore(keyword, name, district, address) {
    const query = asText(keyword).trim().toLowerCase();
    const normalizedName = asText(name).trim().toLowerCase();
    const combined = `${name || ""}${district || ""}${address || ""}`.toLowerCase();
    let score = 0;
    if (!query)
        return score;
    if (normalizedName === query)
        score += 1000;
    if (normalizedName.startsWith(query))
        score += 450;
    if (normalizedName.includes(query))
        score += 320;
    if (combined.includes(query))
        score += 130;
    for (const character of new Set(query.replaceAll(" ", ""))) {
        if (normalizedName.includes(character))
            score += 32;
        else if (combined.includes(character))
            score += 5;
    }
    return score;
}

function endpointContent(kind, name) {
    const label = kind === "start" ? "起" : "终";
    return `<div class="endpoint-marker ${kind}">`
        + `<span class="dot">${label}</span>`
        + `<span class="name">${escapeHtml(name || (kind === "start" ? "起点" : "终点"))}</span>`
        + "</div>";
}

function createOrUpdateEndpointMarker(kind, hasPoint, name, longitude, latitude) {
    const markerName = kind === "start" ? "startMarker" : "endMarker";
    let marker = kind === "start" ? startMarker : endMarker;
    if (!hasPoint || !validCoordinate(longitude, latitude)) {
        if (marker) {
            map.remove(marker);
            marker = null;
        }
    } else if (!marker) {
        marker = new AMap.Marker({
            position: [longitude, latitude],
            anchor: "center",
            offset: new AMap.Pixel(0, -20),
            content: endpointContent(kind, name),
            zIndex: kind === "start" ? 210 : 220
        });
        map.add(marker);
    } else {
        marker.setPosition([longitude, latitude]);
        marker.setContent(endpointContent(kind, name));
    }

    if (markerName === "startMarker")
        startMarker = marker;
    else
        endMarker = marker;
}

function createVehicleElement() {
    const element = document.createElement("div");
    element.className = "vehicle-marker";
    element.innerHTML = '<div class="pulse"></div>'
        + '<div class="arrow-shell"><div class="arrow"></div></div>'
        + '<div class="speed">0 km/min</div>';
    vehicleArrowElement = element.querySelector(".arrow");
    vehicleSpeedElement = element.querySelector(".speed");
    return element;
}

function renderVehicleFrame() {
    pendingVehicleFrame = 0;
    if (!map || !validCoordinate(vehicleState.longitude, vehicleState.latitude))
        return;

    if (!vehicleMarker) {
        vehicleElement = createVehicleElement();
        vehicleMarker = new AMap.Marker({
            position: [vehicleState.longitude, vehicleState.latitude],
            anchor: "center",
            content: vehicleElement,
            zIndex: 500
        });
        map.add(vehicleMarker);
    } else {
        vehicleMarker.setPosition([vehicleState.longitude, vehicleState.latitude]);
    }

    if (vehicleElement)
        vehicleElement.classList.toggle("navigating", vehicleState.navigating);
    if (vehicleArrowElement)
        vehicleArrowElement.style.transform = `rotate(${vehicleState.heading}deg)`;
    if (vehicleSpeedElement) {
        const kilometersPerMinute = Math.abs(vehicleState.speed) / 60;
        vehicleSpeedElement.textContent = `${kilometersPerMinute.toFixed(kilometersPerMinute < 10 ? 1 : 0)} km/min`;
    }

    // 只有用户主动选择“回到车辆”时才保持中心跟车。
    // W/S 本身不会改变比例尺，也不会在用户拖图后抢回地图中心。
    if (followingVehicle && vehicleState.navigating && !userGestureActive) {
        const now = performance.now();
        if (now - lastCenterFollowAt >= CENTER_FOLLOW_INTERVAL_MS) {
            lastCenterFollowAt = now;
            map.setCenter([vehicleState.longitude, vehicleState.latitude], true);
        }
        return;
    }

    // 用户手动拖动后保留其当前视角。只有车辆从当前视野内部真正行驶到
    // 安全边缘时，才进行一次最小幅度平移；不居中、不缩放。
    if (vehicleState.navigating && Math.abs(vehicleState.speed) > 0.01
            && !userGestureActive)
        keepVehicleInsideVisibleEdge();
}

function updateVehicleMarker(longitude, latitude, heading, speed, navigating) {
    vehicleState = {
        longitude: asNumber(longitude, vehicleState.longitude),
        latitude: asNumber(latitude, vehicleState.latitude),
        heading: asNumber(heading, vehicleState.heading),
        speed: asNumber(speed, 0),
        navigating: Boolean(navigating)
    };

    // W/S 只更新车辆位置，绝不自动切换中心跟车模式，也不修改缩放级别。
    if (!vehicleState.navigating) {
        followingVehicle = false;
        manualViewportActive = false;
        edgeFollowArmed = true;
    }

    if (!pendingVehicleFrame)
        pendingVehicleFrame = requestAnimationFrame(renderVehicleFrame);
}

function pixelCoordinate(pixel, getterName, propertyName, fallback = 0) {
    if (!pixel)
        return fallback;
    if (typeof pixel[getterName] === "function")
        return asNumber(pixel[getterName](), fallback);
    return asNumber(pixel[propertyName], fallback);
}

function currentVehiclePixel() {
    if (!map || !validCoordinate(vehicleState.longitude, vehicleState.latitude))
        return null;
    try {
        return map.lngLatToContainer([vehicleState.longitude, vehicleState.latitude]);
    } catch (error) {
        return null;
    }
}

function vehicleInsideSafeViewport() {
    const pixel = currentVehiclePixel();
    if (!pixel || !map)
        return false;
    const size = map.getSize();
    const width = mapSizeValue(size, "getWidth", "width", 1200);
    const height = mapSizeValue(size, "getHeight", "height", 800);
    const x = pixelCoordinate(pixel, "getX", "x", -100000);
    const y = pixelCoordinate(pixel, "getY", "y", -100000);
    return x >= SAFE_EDGE_MARGIN_X
        && x <= width - SAFE_EDGE_MARGIN_X
        && y >= SAFE_EDGE_MARGIN_TOP
        && y <= height - SAFE_EDGE_MARGIN_BOTTOM;
}

function keepVehicleInsideVisibleEdge() {
    if (!map || followingVehicle)
        return;

    const pixel = currentVehiclePixel();
    if (!pixel)
        return;

    const size = map.getSize();
    const width = mapSizeValue(size, "getWidth", "width", 1200);
    const height = mapSizeValue(size, "getHeight", "height", 800);
    const x = pixelCoordinate(pixel, "getX", "x", -100000);
    const y = pixelCoordinate(pixel, "getY", "y", -100000);

    const safeLeft = SAFE_EDGE_MARGIN_X;
    const safeRight = width - SAFE_EDGE_MARGIN_X;
    const safeTop = SAFE_EDGE_MARGIN_TOP;
    const safeBottom = height - SAFE_EDGE_MARGIN_BOTTOM;
    const inside = x >= safeLeft && x <= safeRight && y >= safeTop && y <= safeBottom;

    // 用户可能主动把车辆拖出画面查看别处。此时 W/S 不应立即把地图抢回来；
    // 只有车辆重新进入安全区后，边缘保护才重新启用。
    if (!edgeFollowArmed) {
        if (inside)
            edgeFollowArmed = true;
        return;
    }
    if (inside)
        return;

    let shiftX = 0;
    let shiftY = 0;
    if (x < safeLeft)
        shiftX = x - (safeLeft + SAFE_EDGE_INSET);
    else if (x > safeRight)
        shiftX = x - (safeRight - SAFE_EDGE_INSET);
    if (y < safeTop)
        shiftY = y - (safeTop + SAFE_EDGE_INSET);
    else if (y > safeBottom)
        shiftY = y - (safeBottom - SAFE_EDGE_INSET);

    if (Math.abs(shiftX) < 0.5 && Math.abs(shiftY) < 0.5)
        return;

    const centerPixel = new AMap.Pixel(width / 2 + shiftX, height / 2 + shiftY);
    const targetCenter = map.containerToLngLat(centerPixel);
    if (targetCenter)
        map.setCenter(targetCenter, true);
}

function clearRouteOverlays() {
    const overlays = [];
    if (routeOutline)
        overlays.push(routeOutline);
    if (routePolyline)
        overlays.push(routePolyline);
    if (overlays.length)
        map.remove(overlays);
    routeOutline = null;
    routePolyline = null;
    currentRoutePath = [];
}

function renderRoute(path) {
    clearRouteOverlays();
    currentRoutePath = Array.isArray(path) ? path.filter(point =>
        Array.isArray(point)
        && point.length >= 2
        && validCoordinate(asNumber(point[0], NaN), asNumber(point[1], NaN))) : [];
    if (currentRoutePath.length < 2)
        return;

    routeOutline = new AMap.Polyline({
        path: currentRoutePath,
        isOutline: false,
        strokeColor: "#FFFFFF",
        strokeOpacity: 0.98,
        strokeWeight: 17,
        lineJoin: "round",
        lineCap: "round",
        zIndex: 1000
    });
    routePolyline = new AMap.Polyline({
        path: currentRoutePath,
        isOutline: true,
        outlineColor: "#D9ECFF",
        borderWeight: 2,
        strokeColor: "#1677FF",
        strokeOpacity: 1,
        strokeWeight: 10,
        showDir: true,
        lineJoin: "round",
        lineCap: "round",
        zIndex: 1010
    });
    map.add([routeOutline, routePolyline]);
}

function fitRouteView() {
    if (!map)
        return;
    const overlays = [routeOutline, routePolyline, startMarker, endMarker].filter(Boolean);
    if (!overlays.length)
        return;
    followingVehicle = false;
    // 左侧搜索卡和右侧路线卡都要留出空间，保证蓝色路线不会藏在卡片下面。
    map.setFitView(overlays, false, [82, 390, 96, 390], 18);
}

function mapSizeValue(size, getterName, propertyName, fallback) {
    if (!size)
        return fallback;
    if (typeof size[getterName] === "function")
        return asNumber(size[getterName](), fallback);
    return asNumber(size[propertyName], fallback);
}

function applyPendingPan() {
    pendingPanFrame = 0;
    if (!map)
        return;
    const deltaX = pendingPanX;
    const deltaY = pendingPanY;
    pendingPanX = 0;
    pendingPanY = 0;
    if (Math.abs(deltaX) < 0.01 && Math.abs(deltaY) < 0.01)
        return;

    const size = map.getSize();
    const width = mapSizeValue(size, "getWidth", "width", 1200);
    const height = mapSizeValue(size, "getHeight", "height", 800);
    // 鼠标向右拖时，地图内容应向右移动，因此中心点取当前中心左侧的像素位置。
    const targetPixel = new AMap.Pixel(width / 2 - deltaX, height / 2 - deltaY);
    const targetCenter = map.containerToLngLat(targetPixel);
    if (targetCenter)
        map.setCenter(targetCenter, true);
}

function queuePanBy(deltaX, deltaY) {
    followingVehicle = false;
    manualViewportActive = true;
    pendingPanX += asNumber(deltaX, 0);
    pendingPanY += asNumber(deltaY, 0);
    if (!pendingPanFrame)
        pendingPanFrame = requestAnimationFrame(applyPendingPan);
}

function smoothZoom(delta) {
    if (!map)
        return;
    const target = Math.max(3, Math.min(20, Math.round(map.getZoom()) + delta));
    map.setZoom(target, false, 220);
}

function configureTraffic(enabled) {
    if (!map)
        return;
    if (!trafficLayer)
        trafficLayer = new AMap.TileLayer.Traffic({ zIndex: 12, autoRefresh: true, interval: 180 });
    if (enabled)
        map.add(trafficLayer);
    else
        map.remove(trafficLayer);
}

function searchPlaces(target, keyword, city) {
    if (!autoComplete) {
        bridge.receiveSearchError(target, "地图搜索服务尚未就绪");
        return;
    }
    const query = asText(keyword).trim();
    if (!query) {
        bridge.receiveSearchResults(target, "[]");
        return;
    }

    autoComplete.setCity(asText(city));
    autoComplete.search(query, (status, result) => {
        if (status !== "complete" || !result || !Array.isArray(result.tips)) {
            const message = result && result.info ? asText(result.info) : "高德地点搜索失败";
            bridge.receiveSearchError(target, message);
            return;
        }

        const unique = new Set();
        const places = [];
        for (const tip of result.tips) {
            const coordinate = coordinateOf(tip.location);
            if (!coordinate || !validCoordinate(coordinate[0], coordinate[1]))
                continue;
            const name = asText(tip.name).trim();
            if (!name)
                continue;
            const district = asText(tip.district).trim();
            const address = asText(tip.address).trim();
            const identity = `${tip.id || ""}|${coordinate[0].toFixed(6)},${coordinate[1].toFixed(6)}`;
            if (unique.has(identity))
                continue;
            unique.add(identity);
            places.push({
                id: asText(tip.id),
                name,
                district,
                address,
                longitude: coordinate[0],
                latitude: coordinate[1],
                distance: distanceMeters(vehicleState.longitude,
                                         vehicleState.latitude,
                                         coordinate[0],
                                         coordinate[1]),
                score: relevanceScore(query, name, district, address)
                    + (asText(city).length > 0 && district.includes(asText(city)) ? 260 : 0)
            });
            if (places.length >= 15)
                break;
        }
        bridge.receiveSearchResults(target, JSON.stringify(places));
    });
}

function routeStepPayload(step) {
    const path = [];
    if (Array.isArray(step.path)) {
        for (const point of step.path) {
            const coordinate = coordinateOf(point);
            if (coordinate && validCoordinate(coordinate[0], coordinate[1]))
                path.push(coordinate);
        }
    }
    return {
        instruction: asText(step.instruction),
        road: asText(step.road),
        action: asText(step.action),
        distance: asNumber(step.distance, 0),
        duration: asNumber(step.time, 0),
        path
    };
}

function planRoute(startLongitude, startLatitude, endLongitude, endLatitude) {
    if (!driving) {
        bridge.receiveRouteError("驾车路线服务尚未就绪");
        return;
    }
    const start = [asNumber(startLongitude), asNumber(startLatitude)];
    const end = [asNumber(endLongitude), asNumber(endLatitude)];
    if (!validCoordinate(start[0], start[1]) || !validCoordinate(end[0], end[1])) {
        bridge.receiveRouteError("起点或终点坐标无效");
        return;
    }

    setStatus("正在规划驾车路线…");
    clearRouteOverlays();
    driving.search(start, end, (status, result) => {
        if (status !== "complete" || !result || !Array.isArray(result.routes) || !result.routes.length) {
            const message = result && result.info ? asText(result.info) : "高德未返回可用驾车路线";
            setStatus(message, true);
            bridge.receiveRouteError(message);
            return;
        }

        const route = result.routes[0];
        const steps = Array.isArray(route.steps) ? route.steps.map(routeStepPayload) : [];
        const routePath = [];
        const appendPoint = point => {
            const coordinate = coordinateOf(point);
            if (!coordinate || !validCoordinate(coordinate[0], coordinate[1]))
                return;
            const previous = routePath.length ? routePath[routePath.length - 1] : null;
            if (!previous || previous[0] !== coordinate[0] || previous[1] !== coordinate[1])
                routePath.push(coordinate);
        };
        for (const step of steps) {
            for (const point of step.path)
                appendPoint(point);
        }
        if (routePath.length < 2 && Array.isArray(route.path)) {
            for (const point of route.path)
                appendPoint(point);
        }
        // 极端情况下高德只返回距离/时间而没有步骤 path，也至少先显示起终点连线，
        // 不能让用户看到“规划成功但地图上一条线都没有”。
        if (routePath.length < 2) {
            appendPoint(start);
            appendPoint(end);
        }

        renderRoute(routePath);

        const payload = {
            distance: asNumber(route.distance, 0),
            duration: asNumber(route.time, 0),
            steps
        };
        bridge.receiveRouteResult(JSON.stringify(payload));
        followingVehicle = false;
        requestAnimationFrame(() => requestAnimationFrame(() => {
            fitRouteView();
            setStatus("路线规划完成", false, true);
        }));
    });
}

function focusPoint(longitude, latitude, zoomLevel = 16, animated = true) {
    if (!map || !validCoordinate(longitude, latitude))
        return;
    followingVehicle = false;
    const zoom = Math.max(3, Math.min(20, asNumber(zoomLevel, 16)));
    const position = [longitude, latitude];
    if (!animated) {
        map.setZoomAndCenter(zoom, position, true);
        return;
    }
    map.setZoomAndCenter(zoom, position, false, 280);
    // 动画结束后再做一次立即落点，避免连续的标记更新或 resize 把中心点停在半路。
    window.setTimeout(() => {
        if (map && !followingVehicle)
            map.setZoomAndCenter(zoom, position, true);
    }, 300);
}

function handleEndpointState(hasStart,
                             startName,
                             startLongitude,
                             startLatitude,
                             hasEnd,
                             endName,
                             endLongitude,
                             endLatitude) {
    if (!map)
        return;
    const previousState = endpointState;
    const nextState = {
        hasStart: Boolean(hasStart),
        startLongitude: asNumber(startLongitude),
        startLatitude: asNumber(startLatitude),
        hasEnd: Boolean(hasEnd),
        endLongitude: asNumber(endLongitude),
        endLatitude: asNumber(endLatitude)
    };
    endpointState = nextState;
    createOrUpdateEndpointMarker("start",
                                 hasStart,
                                 startName,
                                 nextState.startLongitude,
                                 nextState.startLatitude);
    createOrUpdateEndpointMarker("end",
                                 hasEnd,
                                 endName,
                                 nextState.endLongitude,
                                 nextState.endLatitude);

    // 端点状态本身也负责聚焦，避免单独的 focus-point 命令因为加载时序丢失。
    if (!vehicleState.navigating && !currentRoutePath.length) {
        let signature = "";
        let longitude = NaN;
        let latitude = NaN;
        const startChanged = nextState.hasStart
            && validCoordinate(nextState.startLongitude, nextState.startLatitude)
            && (!previousState.hasStart
                || previousState.startLongitude !== nextState.startLongitude
                || previousState.startLatitude !== nextState.startLatitude);
        const endChanged = nextState.hasEnd
            && validCoordinate(nextState.endLongitude, nextState.endLatitude)
            && (!previousState.hasEnd
                || previousState.endLongitude !== nextState.endLongitude
                || previousState.endLatitude !== nextState.endLatitude);
        if (endChanged) {
            signature = `end:${nextState.endLongitude.toFixed(6)},${nextState.endLatitude.toFixed(6)}`;
            longitude = nextState.endLongitude;
            latitude = nextState.endLatitude;
        } else if (startChanged) {
            signature = `start:${nextState.startLongitude.toFixed(6)},${nextState.startLatitude.toFixed(6)}`;
            longitude = nextState.startLongitude;
            latitude = nextState.startLatitude;
        }
        if (signature && signature !== endpointFocusSignature) {
            endpointFocusSignature = signature;
            requestAnimationFrame(() => focusPoint(longitude, latitude, 16, true));
        }
    }
}

function handleCommand(command, payload) {
    const commandText = asText(command);
    if (!map) {
        pendingCommands.push([commandText, asText(payload)]);
        return;
    }
    switch (commandText) {
    case "gesture-start":
        userGestureActive = true;
        followingVehicle = false;
        manualViewportActive = true;
        edgeFollowArmed = false;
        map.setDefaultCursor("grabbing");
        break;
    case "gesture-end":
        userGestureActive = false;
        // 只有车辆仍在用户选择的新视野内，才允许随后到达边缘时轻微平移。
        // 若用户刻意把车辆拖出画面，则 W/S 不会立即抢回控制权。
        edgeFollowArmed = vehicleInsideSafeViewport();
        map.setDefaultCursor("grab");
        break;
    case "pan-by": {
        try {
            const pan = JSON.parse(asText(payload));
            queuePanBy(asNumber(pan.deltaX, 0), asNumber(pan.deltaY, 0));
        } catch (error) {
            console.error("pan-by payload invalid", error);
        }
        break;
    }
    case "zoom-at": {
        try {
            const zoomRequest = JSON.parse(asText(payload));
            smoothZoom(asNumber(zoomRequest.direction, 1) >= 0 ? 1 : -1);
        } catch (error) {
            console.error("zoom-at payload invalid", error);
        }
        break;
    }
    case "zoom-in":
        smoothZoom(1);
        break;
    case "zoom-out":
        smoothZoom(-1);
        break;
    case "follow-vehicle":
        followingVehicle = true;
        manualViewportActive = false;
        edgeFollowArmed = true;
        if (validCoordinate(vehicleState.longitude, vehicleState.latitude))
            map.setCenter([vehicleState.longitude, vehicleState.latitude], false, 180);
        break;
    case "drive-follow":
        // 保留旧命令兼容，但不再居中或修改比例尺。
        // W/S 只允许在车辆到达当前视野边缘时触发最小平移。
        if (!manualViewportActive)
            edgeFollowArmed = true;
        break;
    case "fit-route":
        followingVehicle = false;
        fitRouteView();
        break;
    case "traffic":
        configureTraffic(asText(payload) !== "0");
        break;
    case "clear-route":
        followingVehicle = false;
        endpointFocusSignature = "";
        clearRouteOverlays();
        break;
    case "navigation-start":
        followingVehicle = true;
        manualViewportActive = false;
        edgeFollowArmed = true;
        if (validCoordinate(vehicleState.longitude, vehicleState.latitude))
            map.setCenter([vehicleState.longitude, vehicleState.latitude], false, 180);
        renderVehicleFrame();
        break;
    case "focus-point": {
        try {
            const point = JSON.parse(asText(payload));
            focusPoint(asNumber(point.longitude),
                       asNumber(point.latitude),
                       asNumber(point.zoom, 16),
                       true);
        } catch (error) {
            console.error("focus-point payload invalid", error);
        }
        break;
    }
    case "navigation-stop":
        followingVehicle = false;
        manualViewportActive = false;
        edgeFollowArmed = true;
        break;
    default:
        break;
    }
}

function connectBridgeSignals() {
    bridge.searchRequested.connect(searchPlaces);
    bridge.routeRequested.connect(planRoute);
    bridge.endpointStateChanged.connect(handleEndpointState);
    bridge.vehicleStateChanged.connect(updateVehicleMarker);
    bridge.commandRequested.connect(handleCommand);
}

function loadAmapScript() {
    const key = asText(bridge.amapKey).trim();
    const securityCode = asText(bridge.amapSecurityCode).trim();
    if (!key || !securityCode) {
        const message = "config.json 尚未填写高德 JS API Key 和安全密钥";
        setStatus(message, true);
        bridge.reportMapError(message);
        return;
    }

    window._AMapSecurityConfig = { securityJsCode: securityCode };
    const script = document.createElement("script");
    script.type = "text/javascript";
    script.async = true;
    script.onerror = () => {
        const message = "高德 JS API 脚本加载失败，请检查网络和 Key 配置";
        setStatus(message, true);
        bridge.reportMapError(message);
    };
    script.onload = initializeMap;
    script.src = "https://webapi.amap.com/maps?v=2.0"
        + `&key=${encodeURIComponent(key)}`
        + "&plugin=AMap.AutoComplete,AMap.Driving,AMap.Scale";
    document.head.appendChild(script);
}

function initializeMap() {
    try {
        const center = [asNumber(bridge.defaultLongitude, 120.15515),
                        asNumber(bridge.defaultLatitude, 30.27415)];
        map = new AMap.Map("map", {
            viewMode: "3D",
            pitch: 0,
            zoom: 13,
            zooms: [3, 20],
            center,
            mapStyle: "amap://styles/fresh",
            resizeEnable: true,
            animateEnable: true,
            jogEnable: true,
            dragEnable: true,
            zoomEnable: true,
            scrollWheel: true,
            doubleClickZoom: true,
            keyboardEnable: false,
            touchZoom: true,
            touchZoomCenter: 1,
            rotateEnable: false,
            pitchEnable: false,
            showIndoorMap: false,
            showLabel: true,
            skyColor: "#DDEBF2",
            features: ["bg", "road", "building", "point"],
            WebGLParams: { preserveDrawingBuffer: false }
        });

        autoComplete = new AMap.AutoComplete({
            city: asText(bridge.defaultCity),
            citylimit: false
        });
        driving = new AMap.Driving({
            policy: AMap.DrivingPolicy.LEAST_TIME,
            extensions: "all",
            hideMarkers: true,
            showTraffic: true,
            autoFitView: false
        });
        map.addControl(new AMap.Scale());
        configureTraffic(true);

        map.on("dragstart", () => {
            followingVehicle = false;
        });
        map.setDefaultCursor("grab");
        window.addEventListener("resize", () => {
            if (map)
                map.resize();
        });
        map.on("complete", () => {
            if (mapReportedReady)
                return;
            mapReportedReady = true;
            setStatus("高德导航地图已就绪", false, true);
            bridge.reportMapReady();
            bridge.requestInitialState();
            while (pendingCommands.length) {
                const command = pendingCommands.shift();
                handleCommand(command[0], command[1]);
            }
        });
    } catch (error) {
        const message = `地图初始化失败：${error && error.message ? error.message : error}`;
        setStatus(message, true);
        bridge.reportMapError(message);
    }
}

function initializeWebChannel() {
    if (typeof QWebChannel === "undefined" || !window.qt || !qt.webChannelTransport) {
        setStatus("QWebChannel 初始化失败", true);
        return;
    }
    new QWebChannel(qt.webChannelTransport, channel => {
        bridge = channel.objects.navigationBridge;
        if (!bridge) {
            setStatus("未找到 navigationBridge", true);
            return;
        }
        connectBridgeSignals();
        setStatus("正在加载高德 JS API…");
        loadAmapScript();
    });
}

initializeWebChannel();
