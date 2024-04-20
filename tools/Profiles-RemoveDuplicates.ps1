function Remove-Duplicates {
    param (
        [Parameter(Mandatory=$true)]
        [PSCustomObject]$sourceData,
        [Parameter(Mandatory=$true)]
        [PSCustomObject]$targetData
    )

    # Remove duplicates
    $sourceData.PSObject.Properties | ForEach-Object {
      if ($_.Name -eq "type" -or $_.Name -eq "from" -or $_.Name -eq "instantiation") {}
      elseif ($_.Name -in $targetData.PSObject.Properties.Name -and $targetData.$($_.Name) -eq $_.Value) {
            $targetData.PSObject.Properties.Remove($_.Name)
        }
    }

    # Write the updated JSON data back to the second file
    #$targetData | ConvertTo-Json -Depth 100 | Set-Content -Path $jsonFile2

    return $targetData
}

function Add-NonExistingProperties {
  param (
      [Parameter(Mandatory=$true)]
      [hashtable]$sourceData,
      [Parameter(Mandatory=$true)]
      [PSCustomObject]$targetData
  )

  # Iterate over each property in the source data
  $sourceData.Keys | ForEach-Object {
      # If the target data does not contain this property, add it
      if (-not $targetData.PSObject.Properties.Name.Contains($_)) {
          Add-Member -InputObject $targetData -NotePropertyName $_ -NotePropertyValue $sourceData[$_]
      }
      # If the property exists, update it
      else {
          # if the value of sorucedata is not empty, update the targetdata
          if ($sourceData[$_]) {
              $targetData.$_ = $sourceData[$_]
          }
          # else remove the value from the targetdata
          else {
              $targetData.PSObject.Properties.Remove($_)
          } 
      }
  }

  # Return the updated target data
  return $targetData
}

function Format-Json {
  param (
      [Parameter(Mandatory=$true)]
      [PSCustomObject]$data,
      [Parameter(Mandatory=$true)]
      [PSCustomObject]$jsonFile
  )

  # Write the updated JSON data back to the file
  $json = $data | ConvertTo-Json -Depth 100

  $json = $json -replace '":  "', '": "'

  Set-Content -Path $jsonFile -Value $json
}

function Remove-EmptyArrays {
  param (
      [Parameter(Mandatory=$true)]
      [PSCustomObject]$data
  )

  # Recursive function to check and remove empty arrays
  function Check-And-Remove {
      param (
          $obj
      )

      $obj.PSObject.Properties | ForEach-Object {
          if ($_.Value -is [psobject]) {
              Check-And-Remove -obj $_.Value
          } elseif ($_.Value -is [array] -and $_.Value.Count -eq 1 -and $_.Value[0] -eq "") {
              $obj.PSObject.Properties.Remove($_.Name)
          }
      }
  }

  # Call the recursive function
  Check-And-Remove -obj $data

  # Write the updated JSON data back to the file
  return $data
}

# Define the directory and file pattern
$directory = "C:\s\OrcaSlicer2\resources\profiles\Prusa\process"
$filePattern = "*MK3.5*.json"

# define a hashtable with properties to add and remove
$properties = @{
    "type" = "process";
    "setting_id" = "GP004";
    "from" = "system";
    "instantiation" = "true";
    "inherits" = "process_common_mk3is";
    "filename_format" = $null;
}

# Define the first JSON file
$jsonFile1 = Join-Path -Path $directory -ChildPath "process_common_mk3is.json"
# Load the first JSON file
$src = Get-Content -Path $jsonFile1 | ConvertFrom-Json

# Get all files in the directory that match the file pattern
$files = Get-ChildItem -Path $directory -Filter $filePattern

# Invoke the Remove-Duplicates function for each file
foreach ($file in $files) {
    if ($file.FullName -eq $jsonFile1) {
        continue
    }

    # Load the second JSON file
    $target = Get-Content -Path $file.FullName | ConvertFrom-Json

    $properties["name"] = [System.IO.Path]::GetFilenameWithoutExtension($file.Name)
    $properties["print_settings_id"] = $properties["name"] 

    $target = Remove-Duplicates -sourceData $src -targetData $target
    $target = Remove-EmptyArrays -data $target
    $target = Add-NonExistingProperties -sourceData $properties -targetData $target
    Format-Json -data $target -jsonFile $file.FullName
}