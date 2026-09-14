# Ordering the PocketOBI PCB from PCBWay

You don't need to read a schematic. You just send one `.zip` file, copy a
few settings, and PCBWay makes the board. Plan for about 10 minutes.

## Settings to copy

Board: **91 × 34.5 mm · 2 layers**

| Setting | Value |
|---|---|
| Quantity | 5 (the minimum, cheapest) |
| Layers | 2 |
| Dimensions | 91 × 34.5 mm (auto-detected) |
| Material | FR-4 |
| Thickness | 1.6 mm |
| Surface Finish | HASL (default) |
| Solder mask color | your choice (green = fastest) |
| Silkscreen | white |

Leave everything else on its default value — PCBWay is already set to the
cheapest option.

## Step by step

1. **Get the Gerber file.** It's the manufacturing plan of the board:
   [`gerber/PocketOBI-HW-gerbers-2026-08-05.zip`](gerber/PocketOBI-HW-gerbers-2026-08-05.zip).
   > ⚠️ **Do not unzip it.** PCBWay wants the `.zip` as-is.

2. **Go to PCBWay.** Open <https://pcbway.com/g/EhH6Ny> and click
   **"PCB Instant Quote"** (the big quote button). Stay on the **PCB** tab —
   you don't need assembly.

3. **Upload the .zip.** Click **"+ Add Gerber file"** (or "Quick-order PCB")
   and select the `.zip`. PCBWay reads the file and fills in the **size** and
   **layer count** by itself.
   > 💡 If the detected size is ≈ **91 × 34.5 mm** and **2 layers**, you're
   > good — the right file was loaded.

4. **Check the settings.** Compare against the table above. Set **Quantity**
   to **5**.
   > 💡 Solder mask color and finish are cosmetic/comfort only. Green and HASL
   > are the fastest and cheapest.

5. **Add to cart.** Click **"Save to Cart"**. The price shows up. You'll need
   to **log in or create an account** here (just an email + password).

6. **Address and shipping.** Enter your **address**, then pick the **shipping
   method**. A cheaper carrier = a few more days. Shipping is often half the
   price, so take the time to compare.

7. **Pay.** Card or PayPal. Once paid, the order goes to **review by the
   PCBWay engineers** (a few hours). If they have a question about the file,
   they email you — just confirm.
   > ⚠️ Watch your inbox the first few hours. Production doesn't start until
   > you reply.

8. **Receive your boards.** Manufacturing takes 2-3 days, then it ships. You
   get a **tracking number** by email. All that's left is soldering the
   components. 🔧

## Something went wrong?

Post a screenshot of the step that's stuck in the video comments — we'll
figure it out together.

---

PocketOBI · The Repair Forge — based on the Open Battery Information protocol
(Martin Jansson, MIT).
