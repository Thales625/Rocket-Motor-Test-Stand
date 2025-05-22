# ⚖️ Load Cell Interface

Interface em Python para calibrar e visualizar dados de uma célula de carga via porta serial.

## 📚 Como usar

### Instalação

Requisitos:
- `pyserial`
- `matplotlib`

Instale com:

```bash
pip install pyserial matplotlib
```

### Calibrar a célula de carga

Antes de visualizar os dados, é necessário calibrar a célula de carga utilizando uma massa conhecida.

```bash
python main.py calibrate -p "/dev/ttyUSB0" -b 115200 -m 1.0
```

Argumentos:
- `-p`: Porta serial (ex: `/dev/ttyUSB0`, `COM5`)
- `-b`: Baud rate (ex: `9600`, `115200`)
- `-m`: Massa conhecida (em kg)

Durante a calibração:

1. O programa solicitará que você coloque a massa na célula de carga.
2. Pressione Enter para iniciar.
3. Após alguns segundos, o fator de calibração será calculado e salvo no arquivo `settings.json`.

---

### Visualizar dados em tempo real

Depois de calibrado, você pode exibir os dados da célula de carga em tempo real:

```bash
python main.py plot -p "/dev/ttyUSB0" -b 115200
```

Se o `settings.json` já estiver configurado corretamente, você pode simplesmente usar:

```bash
python main.py
```

Isso usará os parâmetros salvos anteriormente.

---

### 🧪 Exemplos rápidos

```bash
# Calibração com 2kg
python main.py calibrate -p COM5 -b 115200 -m 2.0

# Visualizar dados com configuração salva
python main.py
```

---

### 💡 Dica

Você só precisa calibrar novamente se trocar a célula de carga ou a configuração física mudar. O valor de calibração (`calib_factor`) é salvo automaticamente.

---

### 🧾 Autor

Este código foi escrito por **Thales S. Rodrigues**, membro do projeto **UFPEL Rocket Team**.