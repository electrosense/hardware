Sub FixBom
    Dim CurrentSheet
    Dim Iterator
    Dim Component

    If SchServer Is Nothing Then Exit Sub
    Set CurrentSheet = SchServer.GetCurrentSchDocument
    If CurrentSheet Is Nothing Then Exit Sub

    Iterator = CurrentSheet.SchIterator_Create
    If Not (Iterator Is Nothing) Then
        Iterator.AddFilter_ObjectSet(MkSet(eSchComponent))

        ' Start editing
        Call SchServer.ProcessControl.PreProcess(CurrentSheet, "")

        Set Component = Iterator.FirstSchObject
        While Not (Component Is Nothing)
            SetComponent(Component)

            Set Component = Iterator.NextSchObject
        WEnd
        CurrentSheet.SchIterator_Destroy(Iterator)

        ' Stop editing
        Call SchServer.ProcessControl.PostProcess(CurrentSheet, "")
        Call SchServer.RobotManager.SendMessage(CurrentSheet.I_ObjectAddress, c_BroadCast, SCHM_EndModify, c_NoEventData)

    End If

End Sub


Sub SetComponent(Component)
    Select Case Component.Designator.Text
     case "BAT1"
        Component.Comment.Text = "BAT1704WH6327XTSA1"
    case "C1"
        Component.Comment.Text = "100nF"
    case "C2"
        Component.Comment.Text = "100nF"
    case "C3"
        Component.Comment.Text = "100nF"
    case "C4"
        Component.Comment.Text = "100nF"
    case "C5"
        Component.Comment.Text = "100nF"
    case "C6"
        Component.Comment.Text = "100nF"
    case "C19"
        Component.Comment.Text = "100nF"
    case "C29"
        Component.Comment.Text = "100nF"
    case "C31"
        Component.Comment.Text = "100nF"
    case "C34"
        Component.Comment.Text = "100nF"
    case "C35"
        Component.Comment.Text = "100nF"
    case "C36"
        Component.Comment.Text = "100nF"
    case "C37"
        Component.Comment.Text = "100nF"
    case "C38"
        Component.Comment.Text = "100nF"
    case "C41"
        Component.Comment.Text = "100nF"
    case "C44"
        Component.Comment.Text = "100nF"
    case "C45"
        Component.Comment.Text = "100nF"
    case "C51"
        Component.Comment.Text = "100nF"
    case "C54"
        Component.Comment.Text = "100nF"
    case "C68"
        Component.Comment.Text = "100nF"
    case "C69"
        Component.Comment.Text = "100nF"
    case "C74"
        Component.Comment.Text = "100nF"
    case "C7"
        Component.Comment.Text = "1uF"
    case "C14"
        Component.Comment.Text = "1uF"
    case "C15"
        Component.Comment.Text = "1uF"
    case "C8"
        Component.Comment.Text = "10nF"
    case "C9"
        Component.Comment.Text = "10nF"
    case "C10"
        Component.Comment.Text = "10nF"
    case "C11"
        Component.Comment.Text = "10nF"
    case "C12"
        Component.Comment.Text = "10nF"
    case "C13"
        Component.Comment.Text = "10nF"
    case "C49"
        Component.Comment.Text = "10nF"
    case "C50"
        Component.Comment.Text = "10nF"
    case "C16"
        Component.Comment.Text = "22pF"
    case "C17"
        Component.Comment.Text = "22pF"
    case "C52"
        Component.Comment.Text = "22pF"
    case "C53"
        Component.Comment.Text = "22pF"
    case "C55"
        Component.Comment.Text = "22pF"
    case "C56"
        Component.Comment.Text = "22pF"
    case "C58"
        Component.Comment.Text = "22pF"
    case "C57"
        Component.Comment.Text = "22pF"
    case "C63"
        Component.Comment.Text = "22pF"
    case "C18"
        Component.Comment.Text = "12nF"
    case "C20"
        Component.Comment.Text = "820pF"
    case "C47"
        Component.Comment.Text = "10nF"
    case "C73"
        Component.Comment.Text = "10nF"
    case "C75"
        Component.Comment.Text = "10nF"
    case "C80"
        Component.Comment.Text = "10nF"
    case "C81"
        Component.Comment.Text = "10nF"
    case "C39"
        Component.Comment.Text = "10pF"
    case "C40"
        Component.Comment.Text = "10pF"
    case "C42"
        Component.Comment.Text = "10uF"
    case "C43"
        Component.Comment.Text = "10uF"
    case "C59"
        Component.Comment.Text = "100pF"
    case "C67"
        Component.Comment.Text = "100pF"
    case "C66"
        Component.Comment.Text = "100pF"
    case "C60"
        Component.Comment.Text = "0.2pF"
    case "C76"
        Component.Comment.Text = "47pF"
    case "C78"
        Component.Comment.Text = "47pF"
    case "C79"
        Component.Comment.Text = "47pF"
    case "C82"
        Component.Comment.Text = "47pF"
    case "C83"
        Component.Comment.Text = "47pF"
    case "C64"
        Component.Comment.Text = "47pF"
    case "C65"
        Component.Comment.Text = "47pF"
    case "C70"
        Component.Comment.Text = "47pF"
    case "C71"
        Component.Comment.Text = "47pF"
    case "C72"
        Component.Comment.Text = "47pF"
    case "C85"
        Component.Comment.Text = "47pF"
    case "C86"
        Component.Comment.Text = "47pF"
    case "C77"
        Component.Comment.Text = "6.8pF"
    case "C84"
        Component.Comment.Text = "9.1pF"
    case "D2"
        Component.Comment.Text = "ESD0P8RFL"
    case "D3"
        Component.Comment.Text = "ESD0P8RFL"
    case "D4"
        Component.Comment.Text = "ESD0P8RFL"
    case "D5"
        Component.Comment.Text = "ESD0P8RFL"
    case "F1"
        Component.Comment.Text = "DPX165850DT-8017A1"
    case "L1"
        Component.Comment.Text = "Ferrite"
    case "L2"
        Component.Comment.Text = "Ferrite"
    case "L5"
        Component.Comment.Text = "Ferrite"
    case "L4"
        Component.Comment.Text = "220nH"
    case "L6"
        Component.Comment.Text = "470nH"
    case "L7"
        Component.Comment.Text = "470nH"
    case "L8"
        Component.Comment.Text = "470nH"
    case "L9"
        Component.Comment.Text = "470nH"
    case "L10"
        Component.Comment.Text = "470nH"
    case "LOCK"
        Component.Comment.Text = "HSMG-C190"
    case "MIX"
        Component.Comment.Text = "HSMY-C190"
    case "R1"
        Component.Comment.Text = "5K1"
    case "R12"
        Component.Comment.Text = "0"
    case "R3"
        Component.Comment.Text = "270"
    case "R4"
        Component.Comment.Text = "50"
    case "R5"
        Component.Comment.Text = "50"
    case "R9"
        Component.Comment.Text = "50"
    case "R29"
        Component.Comment.Text = "750"
    case "R20"
        Component.Comment.Text = "750"
    case "R21"
        Component.Comment.Text = "750"
    case "R22"
        Component.Comment.Text = "750"
    case "R10"
        Component.Comment.Text = "240"
    case "R11"
        Component.Comment.Text = "30.1"
    case "R23"
        Component.Comment.Text = "1K5"
    case "R24"
        Component.Comment.Text = "1K5"
    case "R31"
        Component.Comment.Text = "1K5"
    case "R33"
        Component.Comment.Text = "10K"
    case "R35"
        Component.Comment.Text = "10K"
    case "R34"
        Component.Comment.Text = "51"
    case "R37"
        Component.Comment.Text = "51"
    case "R38"
        Component.Comment.Text = "51"
    case "R25"
        Component.Comment.Text = "51"
    case "R26"
        Component.Comment.Text = "51"
    case "R8"
        Component.Comment.Text = "51"
    case "R27"
        Component.Comment.Text = "51"
    case "R28"
        Component.Comment.Text = "51"
    case "R30"
        Component.Comment.Text = "51"
    case "R32"
        Component.Comment.Text = "51"
    case "R7"
        Component.Comment.Text = "51"
    case "R36"
        Component.Comment.Text = "390"
    case "R39"
        Component.Comment.Text = "91"
    case "T2"
        Component.Comment.Text = "BC856"
    case "U1"
        Component.Comment.Text = "MAX2870"
    case "U5"
        Component.Comment.Text = "MCP9804"
    case "U7"
        Component.Comment.Text = "STM32F103C8T6"
    case "U8"
        Component.Comment.Text = "IP4220CZ6"
    case "U9"
        Component.Comment.Text = "LD1117-SOT223"
    case "U10"
        Component.Comment.Text = "SKY13322-375LF"
    case "U13"
        Component.Comment.Text = "SKY13322-375LF"
    case "U11"
        Component.Comment.Text = "GRF4001"
    case "U12"
        Component.Comment.Text = "LTC5549"
    case "U14"
        Component.Comment.Text = "TCA6408A"
    case "U15"
        Component.Comment.Text = "SA612"
    case "U16"
        Component.Comment.Text = "RF3025"
    case "USB"
        Component.Comment.Text = "HSMH-C190"
    case "USB-B"
        Component.Comment.Text = "USB-B"
    case "X1"
        Component.Comment.Text = "X1G004691000112"
    case "X2"
        Component.Comment.Text = "KTL/5x3.2mm16MHz"
    case else
        Component.Comment.Text = "DNP"
    End Select
End sub
