# Candidate Handwritten Derivation Template & Reference Appendix

**Assignment:** PASS Science Works — R&D Internship 2026–2027  
**Module:** Neural Computation / Analytical Backpropagation  
**Instruction:** Review the mathematical proof below, verify each step by hand, and transcribe the final derivation onto physical paper for interview submission.

---

## 1. Single-Sample Problem Definition

Given:
- Input sample: $x = 1.0$
- Ground-truth target: $y = 2.0$
- Learning rate: $\eta = 0.1$
- Initial parameter set $\theta_0$:
  $$w = 0.5, \quad b = 0.0, \quad v = 1.0, \quad c = 0.0$$

Network Architecture:
1. Pre-activation: $z = w \cdot x + b$
2. Hidden layer: $h = \tanh(z)$
3. Output prediction: $\hat{y} = v \cdot h + c$
4. Scalar Quadratic Loss: $J = \frac{1}{2}(\hat{y} - y)^2$

---

## 2. Forward Pass Evaluation at $\theta_0$

1. $z = (0.5)(1.0) + 0.0 = 0.500000$
2. $h = \tanh(0.5) = \frac{e^{0.5} - e^{-0.5}}{e^{0.5} + e^{-0.5}} \approx 0.462117$
3. $\hat{y} = (1.0)(0.462117) + 0.0 = 0.462117$
4. Prediction error: $e = \hat{y} - y = 0.462117 - 2.0 = -1.537883$
5. Initial Loss:
   $$J_0 = \frac{1}{2}(-1.537883)^2 \approx 1.182562$$

---

## 3. Analytical Derivation of Gradients (Chain Rule)

### Derivative of Activation Function:
$$\frac{dh}{dz} = \frac{d}{dz}\tanh(z) = 1 - \tanh^2(z) = 1 - h^2$$

### Output Layer Gradients:
$$\frac{\partial J}{\partial \hat{y}} = \hat{y} - y$$

$$\frac{\partial J}{\partial c} = \frac{\partial J}{\partial \hat{y}} \cdot \frac{\partial \hat{y}}{\partial c} = (\hat{y} - y) \cdot 1 = \hat{y} - y$$
**Numerical Value:**
$$\frac{\partial J}{\partial c} = -1.537883$$

$$\frac{\partial J}{\partial v} = \frac{\partial J}{\partial \hat{y}} \cdot \frac{\partial \hat{y}}{\partial v} = (\hat{y} - y) \cdot h$$
**Numerical Value:**
$$\frac{\partial J}{\partial v} = (-1.537883)(0.462117) = -0.710682$$

### Hidden Layer Gradients:
$$\frac{\partial J}{\partial h} = \frac{\partial J}{\partial \hat{y}} \cdot \frac{\partial \hat{y}}{\partial h} = (\hat{y} - y) \cdot v$$

$$\frac{\partial J}{\partial b} = \frac{\partial J}{\partial h} \cdot \frac{dh}{dz} \cdot \frac{\partial z}{\partial b} = (\hat{y} - y) \cdot v \cdot (1 - h^2) \cdot 1$$
**Numerical Value:**
$$1 - h^2 = 1 - (0.462117)^2 = 1 - 0.213552 = 0.786448$$
$$\frac{\partial J}{\partial b} = (-1.537883)(1.0)(0.786448) = -1.210080$$

$$\frac{\partial J}{\partial w} = \frac{\partial J}{\partial h} \cdot \frac{dh}{dz} \cdot \frac{\partial z}{\partial w} = (\hat{y} - y) \cdot v \cdot (1 - h^2) \cdot x$$
Since $x = 1.0$:
$$\frac{\partial J}{\partial w} = \frac{\partial J}{\partial b} \cdot 1.0 = -1.210080$$

---

## 4. Simultaneous Step 1 Parameter Update

All four parameters are updated simultaneously using the original parameter values $\theta_0$ for all gradients:

$$\theta_1 = \theta_0 - \eta \frac{\partial J}{\partial \theta}$$

1. $w_1 = 0.5 - (0.1)(-1.210080) = 0.5 + 0.121008 = \mathbf{0.621008}$
2. $b_1 = 0.0 - (0.1)(-1.210080) = 0.0 + 0.121008 = \mathbf{0.121008}$
3. $v_1 = 1.0 - (0.1)(-0.710682) = 1.0 + 0.071068 = \mathbf{1.071068}$
4. $c_1 = 0.0 - (0.1)(-1.537883) = 0.0 + 0.153788 = \mathbf{0.153788}$

### Verification of Gradient Direction:
Because $\hat{y}_0 = 0.462 < y = 2.0$, the network under-predicted.
All four partial derivatives are negative ($\frac{\partial J}{\partial \theta} < 0$).
Subtracting a negative gradient increases all four parameters, which increases $h$, increases $\hat{y}$, and drives $\hat{y}$ toward $y = 2.0$, successfully reducing loss.
Loss after Step 1:
$$h_1 = \tanh(0.621008 \cdot 1 + 0.121008) = \tanh(0.742016) \approx 0.630422$$
$$\hat{y}_1 = (1.071068)(0.630422) + 0.153788 \approx 0.829019$$
$$J_1 = \frac{1}{2}(0.829019 - 2.0)^2 = \frac{1}{2}(-1.170981)^2 \approx \mathbf{0.685598}$$
Loss decreased from $1.182562 \rightarrow 0.685598$ in a single simultaneous step!
