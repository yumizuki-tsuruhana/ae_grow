/*
    YT Grow — ScriptUI Panel for After Effects
    Lightweight & powerful grow/shrink effect using built-in AE effects.
    No compilation needed — just drop into ScriptUI Panels folder.
*/

(function(thisObj) {

    function buildUI(thisObj) {
        var win = (thisObj instanceof Panel)
            ? thisObj
            : new Window("palette", "YT Grow", undefined, { resizeable: true });

        win.orientation = "column";
        win.alignChildren = ["fill", "top"];
        win.spacing = 4;
        win.margins = 10;

        // --- Header ---
        var header = win.add("statictext", undefined, "YT Grow");
        header.alignment = ["center", "top"];
        header.graphics.font = ScriptUI.newFont("Arial", "Bold", 14);

        win.add("panel", undefined, "").preferredSize.height = 2;

        // --- Radius ---
        var radiusGrp = win.add("group");
        radiusGrp.alignment = ["fill", "top"];
        radiusGrp.add("statictext", undefined, "Radius");
        var radiusSlider = radiusGrp.add("slider", undefined, 10, 0, 200);
        radiusSlider.preferredSize.width = 140;
        var radiusVal = radiusGrp.add("edittext", undefined, "10");
        radiusVal.preferredSize.width = 40;
        radiusSlider.onChanging = function() {
            radiusVal.text = Math.round(this.value).toString();
        };
        radiusVal.onChanging = function() {
            var v = parseInt(this.text);
            if (!isNaN(v)) radiusSlider.value = Math.max(0, Math.min(200, v));
        };

        // --- Softness ---
        var softGrp = win.add("group");
        softGrp.alignment = ["fill", "top"];
        softGrp.add("statictext", undefined, "Softness");
        var softSlider = softGrp.add("slider", undefined, 0, 0, 100);
        softSlider.preferredSize.width = 140;
        var softVal = softGrp.add("edittext", undefined, "0");
        softVal.preferredSize.width = 40;
        softSlider.onChanging = function() {
            softVal.text = Math.round(this.value).toString();
        };
        softVal.onChanging = function() {
            var v = parseInt(this.text);
            if (!isNaN(v)) softSlider.value = Math.max(0, Math.min(100, v));
        };

        // --- Mode ---
        var modeGrp = win.add("group");
        modeGrp.alignment = ["fill", "top"];
        modeGrp.add("statictext", undefined, "Mode");
        var modeDrop = modeGrp.add("dropdownlist", undefined,
            ["Grow", "Shrink", "Grow + Shrink"]);
        modeDrop.selection = 0;

        // --- Shape ---
        var shapeGrp = win.add("group");
        shapeGrp.alignment = ["fill", "top"];
        shapeGrp.add("statictext", undefined, "Shape");
        var shapeDrop = shapeGrp.add("dropdownlist", undefined,
            ["Circle (Disc)", "Square", "Diamond (Cross)"]);
        shapeDrop.selection = 0;

        // --- Channel ---
        var chanGrp = win.add("group");
        chanGrp.alignment = ["fill", "top"];
        chanGrp.add("statictext", undefined, "Channel");
        var chanDrop = chanGrp.add("dropdownlist", undefined,
            ["Alpha", "All (RGB + Alpha)"]);
        chanDrop.selection = 0;

        // --- Iterations (for large radius) ---
        var iterGrp = win.add("group");
        iterGrp.alignment = ["fill", "top"];
        iterGrp.add("statictext", undefined, "Quality");
        var iterDrop = iterGrp.add("dropdownlist", undefined,
            ["Fast (1x)", "Medium (2x)", "High (3x)"]);
        iterDrop.selection = 0;

        win.add("panel", undefined, "").preferredSize.height = 2;

        // --- Apply button ---
        var applyBtn = win.add("button", undefined, "Apply to Selected Layer");
        applyBtn.alignment = ["center", "bottom"];
        applyBtn.preferredSize.height = 32;

        applyBtn.onClick = function() {
            applyGrow(
                Math.round(radiusSlider.value),
                Math.round(softSlider.value),
                modeDrop.selection.index,
                shapeDrop.selection.index,
                chanDrop.selection.index,
                iterDrop.selection.index + 1
            );
        };

        if (win instanceof Window) {
            win.center();
            win.show();
        } else {
            win.layout.layout(true);
        }

        return win;
    }

    // Minimax operation mapping
    // AE Minimax "Operation":  1=Minimum, 2=Maximum, 3=Minimum then Maximum, 4=Maximum then Minimum
    // AE Minimax "Direction":  1=Horizontal and Vertical, 2=Just Horizontal, 3=Just Vertical

    function applyGrow(radius, softness, modeIdx, shapeIdx, chanIdx, iterations) {
        var comp = app.project.activeItem;
        if (!comp || !(comp instanceof CompItem)) {
            alert("Please open a composition first.");
            return;
        }

        var layers = comp.selectedLayers;
        if (layers.length === 0) {
            alert("Please select a layer.");
            return;
        }

        app.beginUndoGroup("YT Grow");

        try {
            for (var i = 0; i < layers.length; i++) {
                applyToLayer(layers[i], radius, softness, modeIdx, shapeIdx, chanIdx, iterations);
            }
        } catch (e) {
            alert("Error: " + e.toString());
        }

        app.endUndoGroup();
    }

    function applyToLayer(layer, radius, softness, modeIdx, shapeIdx, chanIdx, iterations) {
        if (radius <= 0) return;

        // Determine Minimax operation based on mode
        // Grow = Maximum (2), Shrink = Minimum (1), Grow+Shrink = Max then Min (4)
        var operation;
        switch (modeIdx) {
            case 0: operation = 2; break; // Grow -> Maximum
            case 1: operation = 1; break; // Shrink -> Minimum
            case 2: operation = 4; break; // Grow+Shrink -> Maximum then Minimum
        }

        // Minimax shape: for Circle we use H&V combined, for others specific directions
        // The Minimax "Direction" parameter doesn't map to shape directly.
        // To get different shapes:
        //   Circle(Disc): apply H&V together (Direction=1) — this is the default disc shape
        //   Square: apply H then V separately (two effects)
        //   Diamond(Cross): apply H&V together, AE Minimax does this as diamond when Direction=1

        // Minimax channel mapping
        // 1=Color and Alpha, 2=Color Only, 3=Alpha Only
        var channel = (chanIdx === 0) ? 3 : 1; // Alpha only or All

        // For large radius, split across iterations
        var perIter = Math.ceil(radius / iterations);

        if (shapeIdx === 1) {
            // Square: apply separate H and V passes
            for (var it = 0; it < iterations; it++) {
                var r = (it === iterations - 1)
                    ? radius - perIter * (iterations - 1)
                    : perIter;
                if (r <= 0) continue;

                // Horizontal pass
                var fxH = layer.Effects.addProperty("ADBE Minimax");
                fxH.name = "YT Grow H" + (iterations > 1 ? " " + (it + 1) : "");
                fxH.property("ADBE Minimax-0001").setValue(r);       // Radius
                fxH.property("ADBE Minimax-0003").setValue(operation); // Operation
                fxH.property("ADBE Minimax-0002").setValue(channel);  // Channel
                fxH.property("ADBE Minimax-0004").setValue(2);        // Direction: Horizontal

                // Vertical pass
                var fxV = layer.Effects.addProperty("ADBE Minimax");
                fxV.name = "YT Grow V" + (iterations > 1 ? " " + (it + 1) : "");
                fxV.property("ADBE Minimax-0001").setValue(r);
                fxV.property("ADBE Minimax-0003").setValue(operation);
                fxV.property("ADBE Minimax-0002").setValue(channel);
                fxV.property("ADBE Minimax-0004").setValue(3);        // Direction: Vertical
            }
        } else {
            // Circle (Disc) or Diamond: single H&V pass
            for (var it = 0; it < iterations; it++) {
                var r = (it === iterations - 1)
                    ? radius - perIter * (iterations - 1)
                    : perIter;
                if (r <= 0) continue;

                var fx = layer.Effects.addProperty("ADBE Minimax");
                fx.name = "YT Grow" + (iterations > 1 ? " " + (it + 1) : "");
                fx.property("ADBE Minimax-0001").setValue(r);
                fx.property("ADBE Minimax-0003").setValue(operation);
                fx.property("ADBE Minimax-0002").setValue(channel);
                fx.property("ADBE Minimax-0004").setValue(1);         // Direction: H&V
            }
        }

        // Add softness via Gaussian Blur on the result
        if (softness > 0) {
            var blur = layer.Effects.addProperty("ADBE Gaussian Blur 2");
            blur.name = "YT Grow Softness";
            blur.property("ADBE Gaussian Blur 2-0001").setValue(softness * 0.5);
            blur.property("ADBE Gaussian Blur 2-0002").setValue(3); // Repeat Edge Pixels
        }
    }

    buildUI(thisObj);

})(this);
