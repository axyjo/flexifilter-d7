<?php
// $Id: flexifilter.admin.inc,v 1.14.2.3 2008/11/22 22:13:33 cwgordon7 Exp $

/**
 * Generates the form for the admin overview page
 */
function flexifilter_filter_list_form() {
  drupal_set_title("Flexifilters overview");
  $path = drupal_get_path('module', 'flexifilter') .'/flexifilter.css';
  drupal_add_css($path, 'module', 'all', FALSE);

  $form = array();
  $form['enabled_header'] = array(
    '#value' => '<h3>'. t('Enabled Flexifilters') .'</h3>',
    '#suffix' => t('If a flexifilter is enabled, then it can be used within input formats, however there can only be 128 enabled at any one time.') .'<br/>',
  );
  $form['enabled'] = flexifilter_filter_list_form_table(TRUE);

  $form['disabled_header'] = array(
    '#value' => '<h3>'. t('Disabled Flexifilters') .'</h3>',
    '#suffix' => t('If a flexifilter is disabled, then it cannot be used within input formats, although it can be used within other flexifilters.') .'<br/>',
  );
  $form['disabled'] = flexifilter_filter_list_form_table(FALSE);

  return $form;
}

/**
 * Helper function for flexifilter_filter_list_form. Returns the table for the overview page.
 *
 * @param $enabled
 *   Is TRUE if we're theming the currently enabled filters.s
 */
function flexifilter_filter_list_form_table($enabled) {
  $show_disable_link = $enabled;
  $show_enable_link  = !$enabled && flexifilter_get_number_enabled_filters() < FLEXIFILTER_MAX_FILTERS;
  $header            = array(t('Label'), t('Description'), t('Edit'), t('Delete'), t('Preview'), t($enabled ? 'Disable' : 'Enable'));
  $rows              = array();
  $filters           = flexifilter_get_filters();
  foreach ($filters as $id => $filter) {
    if ($filter['enabled'] == $enabled) {
      $row   = array($filter['label'], $filter['description']);
      $row[] = l(t('Edit'), 'admin/build/flexifilters/'. $id .'/edit', array(), drupal_get_destination());
      $row[] = l(t('Delete'), 'admin/build/flexifilters/'. $id .'/delete', array(), drupal_get_destination());
      $row[] = l(t('Preview'), 'admin/build/flexifilters/'. $id .'/preview', array(), drupal_get_destination());  
      if ($show_disable_link) {
        $row[] = l(t('Disable'), 'admin/build/flexifilters/'. $id .'/disable', array(), drupal_get_destination());
      }
      else if ($show_enable_link) {
        $row[] = l(t('Enable'), 'admin/build/flexifilters/'. $id .'/enable', array(), drupal_get_destination());
      }
      else {
        $row[] = '';
      }
      $rows[] = $row;
    }
  }
  if (count($rows)) {
    return array('#value' => theme('table', $header, $rows, array('class' => 'flexifilters-list')));
  }
  else {
    return array('#value' => t('None.'));
  }
}

/**
 * Helper function for flexifilter_filter_edit_form; removes the automatic
 * naming of submit controls.
 * Our forms are complex and have submit controls nested within fieldsets,
 * and when pressed, we want the 'op' field to be set within the fieldset,
 * not at the top level, so we have to undo the automatic naming of submit
 * fields.
 * This function is a brutal rip of code from _form_builder_handle_input_element
 *
 * @param $form A form element to reset to the normal name.
 *
 * @return The updated form element
 */
function flexifilter_undo_submit_default_name($form) {
  $name = array_shift($form['#parents']);
  $form['#name'] = $name;
  if ($form['#type'] == 'file') {
    // To make it easier to handle $_FILES in file.inc, we place all
    // file fields in the 'files' array. Also, we do not support
    // nested file names.
    $form['#name'] = 'files['. $form['#name'] .']';
  }
  elseif (count($form['#parents'])) {
    $form['#name'] .= '['. implode('][', $form['#parents']) .']';
  }
  array_unshift($form['#parents'], $name);
  return $form;
}

/**
 * Helper function for flexifilter_filter_edit_components_data; converts a condition
 * array from a flexifilter into a form data styled array.
 */
function flexifilter_filter_edit_condition_data($data) {
  $condition = array(
    'class' => $data['class'],
  );
  foreach ($data['settings'] as $key => $value) {
    if ($key == 'conditions') {
      foreach ($value as $cond_key => $cond_value) {
        $condition['condition_'. $cond_key] = flexifilter_filter_edit_condition_data($cond_value);
      }
    }
    else {
      $condition['setting_'. $key] = $value;
    }
  }
  return $condition;
}

/**
 * Helper function for flexifilter_filter_edit_form; converts a components
 * array from a flexifilter into a form data styled array.
 */
function flexifilter_filter_edit_components_data($components, $id_prefix = NULL) {
  $data = array();
  if (isset($components['id_prefix'])) {
    $id_prefix = $components['id_prefix'];
    $data['id_prefix'] = $id_prefix;
    unset($components['id_prefix']);
  }
  if (isset($components['id_next'])) {
    $data['id_next'] = $components['id_next'];
    unset($components['id_next']);
  }
  foreach ($components as $n => $component) {
    $us = array();
    foreach ($component['settings'] as $key => $value) {
      if ($key == 'components') {
        $us['components'] = flexifilter_filter_edit_components_data($value, $id_prefix);
      }
      elseif ($key == 'condition') {
        $us['condition'] = flexifilter_filter_edit_condition_data($value);
      }
      elseif ($key == 'step') {
        $us[$key] = $value;
      }
      else {
        $us['setting_'. $key] = $value;
      }
    }
    $us['weight'] = $n;
    $us = array_merge($us, $component);
    unset($us['settings']);
    $id = $us['id'];
    unset($us['id']);
    $data[$id_prefix . $id] = $us;
  }
  return $data;
}

/**
 * Creates an FAPI values array for a newly created component
 *
 * @param $component_class The component class of the new component
 */
function flexifilter_filter_edit_form_new_component_data($component_class) {
  $components = flexifilter_get_component_list();
  $component  = $components[$component_class];
  $data       = array(
    'class' => $component_class,
    'weight' => 10000,
    'is_new' => true,
  );
  if ($component['step'] == 'either') {
    $data['step'] = 'process';
  }
  if ($component['is_container']) {
    if ($component['contains_condition']) {
      $data['condition'] = array('class' => '');
    }
    if ($component['contains_components']) {
      $data['components'] = array();
    }
  }
  return $data;
}

/**
 * Creates an FAPI values array for a newly created condition
 *
 * @param $condition_class The condition class of the new condition
 */
function flexifilter_filter_edit_form_new_condition_data($condition_class) {
  $conditions = flexifilter_get_condition_list();
  $condition  = $conditions[$condition_class];
  $data       = array(
    'class' => $condition_class,
    'is_new' => TRUE,
    'weight' => 10000,
  );
  return $data;
}

/**
 * Helper function for flexifilter_filter_edit_form_component; creates an FAPI form table
 * for a condition.
 */
function flexifilter_filter_edit_form_condition($data, $show_advanced) {
  if ($data['class'] == '') {
    $form = array(
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => (isset($data['op']) || isset($data['op_c']) || isset($data['is_new'])) ? FALSE : TRUE,
      '#tree' => TRUE,
      '#title' => t('No condition'),
      '#description' => t('There is no condition set. This will probably cause the component to do nothing, as "No Condition" evaluates to false.'),
    );
    $form['class'] = array('#type' => 'hidden', '#value' => '');
    return $form;
  }

  $conditions = flexifilter_get_condition_list();
  $condition  = $conditions[$data['class']];
  $form       = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => (isset($data['op']) || isset($data['is_new'])) ? FALSE : TRUE,
    '#tree' => TRUE,
    '#title' => $condition['label'],
    '#description' => $condition['description'],
  );
  $form['class'] = array('#type' => 'hidden', '#value' => $data['class']);
  $custom_settings = array();
  foreach ($data as $key => $value) {
    if (strncmp($key, 'setting_', 8) == 0) {
      $custom_settings[substr($key, 8)] = $value;
    }
  }
  $custom_elements = flexifilter_invoke_condition(array('class' => $data['class'], 'settings' => $custom_settings), 'settings');
  if (is_array($custom_elements)) {
    foreach ($custom_elements as $key => $element) {
      $form['setting_'. $key] = $element;
    }
  }
  if ($condition['is_container']) {
    if ($condition['contains_conditions']) {
      $conditions = flexifilter_get_condition_list();

      $condition_max_id = 0;
      foreach ($data as $key => $sub_condition) {
        if (strncmp($key, 'condition_', 10) == 0) {
          $condition_max_id = max($condition_max_id, 1 + (int)substr($key, 10));
          if (!isset($sub_condition['op'])) {
            $form[$key] = flexifilter_filter_edit_form_condition($sub_condition, $show_advanced);
          }
          else {
            $form['#collapsed'] = FALSE;
          }
        }
      }
      if (isset($data['op_c'])) {
        $form['condition_'. $condition_max_id] = flexifilter_filter_edit_form_condition(flexifilter_filter_edit_form_new_condition_data($data['add_condition']), $show_advanced);
      }

      $form['add_condition'] = array(
        '#type' => 'select',
        '#title' => t('Add condition'),
        '#options' => flexifilter_get_grouped_labels($conditions),
        '#description' => t('Adds a new condition wthin this one.'),
      );
      $form['op_c'] = array(
        '#type' => 'submit',
        '#value' => t('Add'),
        '#process' => array('flexifilter_undo_submit_default_name', 'form_expand_ahah'),
      );
      // Add some kind of seperator, otherwise this add button and the remove button are put onto the same line
      // which is just a little bit confusing to the user. Hence we use a horizontal rule to seperate them alot.
      $form['end_add_cond'] = array('#type' => 'markup', '#value' => '<br/><br/><hr/><br/>');
    }
  }
  $form['op'] = array(
    '#type' => 'submit',
    '#value' => t('Remove condition'),
    '#process' => array('flexifilter_undo_submit_default_name', 'form_expand_ahah'),
  );

  return $form;
}

/**
 * Helper function for flexifilter_filter_edit_form_components; generates an FAPI array for a
 * single component.
 */
function flexifilter_filter_edit_form_component($data, $new_weight, $is_first, $is_last, &$data_base, $show_advanced) {
  $components = flexifilter_get_component_list();
  $component  = $components[$data['class']];
  $form       = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => (isset($data['op']) || isset($data['op_c']) || isset($data['is_new'])) ? FALSE : TRUE,
    '#tree' => TRUE,
    '#title' => $component['label'],
    '#description' => $component['description'],
  );
  $form['class'] = array(
    '#type' => 'hidden',
    '#value' => $data['class'],
  );
  $form['weight'] = array(
    '#type' => 'hidden',
    '#value' => $new_weight,
  );
  $custom_settings = array();
  foreach ($data as $key => $value) {
    if (strncmp($key, 'setting_', 8) == 0) {
      $custom_settings[substr($key, 8)] = $value;
    }
  }
  $custom_elements = flexifilter_invoke_component($component, 'settings', $custom_settings);
  if (is_array($custom_elements)) {
    foreach ($custom_elements as $key => $element) {
      $form['setting_'. $key] = $element;
    }
  }
  if ($component['is_container']) {
    if ($component['contains_condition']) {
      if (isset($data['op_c'])) {
        $data['condition'] = flexifilter_filter_edit_form_new_condition_data($data['set_condition']);
      }
      if (isset($data['condition']) && isset($data['condition']['op'])) {
        $data['condition'] = array(
          'class' => '',
        );
        $form['#collapsed'] = FALSE;
      }
      $form['condition'] = flexifilter_filter_edit_form_condition($data['condition'], $show_advanced);
      if ($form['condition']['#collapsed'] == FALSE) {
        $form['#collapsed'] = FALSE;
      }
      $conditions = flexifilter_get_condition_list($show_advanced);
      $form['set_condition'] = array(
        '#type' => 'select',
        '#title' => t('Change condition'),
        '#options' => flexifilter_get_grouped_labels($conditions),
        '#description' => t('Changes the condition assocated with this component.'),
      );
      $form['op_c'] = array(
        '#type' => 'submit',
        '#value' => t('Change'),
        '#process' => array('flexifilter_undo_submit_default_name', 'form_expand_ahah'),
      );
    }
    if ($component['contains_components']) {
      $form['components'] = flexifilter_filter_edit_form_components($data['components'], $data_base, $show_advanced);
      if ($form['components']['#collapsed'] == FALSE) {
        $form['#collapsed'] = FALSE;
      }
    }
  }
  if ($component['step'] == 'either') {
    if ($show_advanced) {
      $form['step'] = array(
        '#type' => 'select',
        '#title' => t('Step'),
        '#options' => array(
          'process' => t('Processing'),
          'prepare' => t('Preparation'),
        ),
        '#description' => t('The filtering step to perform this action in.'),
        '#default_value' => $data['step'],
      );
    }
    else {
      $form['step'] = array(
        '#type' => 'hidden',
        '#value' => $data['step'],
      );
    }
  }
  $form['move'] = array(
    '#type' => 'select',
    '#title' => t('Re/move this'),
    '#options' => array(),
    // If the element at the bottom is moved to the top, then the #value of 'top' will no longer be in the downdown
    // and the FAPI throws a nasty validation error. Hence mark it already validated and we will do our own validation.
    '#validated' => true,
  );
  if (!$is_first) {
    $form['move']['#options']['top'] = t('Move to top');
    $form['move']['#options']['up'] = t('Move up');
  }
  if (!$is_last) {
    $form['move']['#options']['down'] = t('Move down');
    $form['move']['#options']['bottom'] = t('Move to bottom');
  }
  $form['move']['#options']['remove'] = t('Remove');
  $form['op'] = array(
    '#type' => 'submit',
    '#value' => t('Re/move'),
    '#process' => array('flexifilter_undo_submit_default_name', 'form_expand_ahah'),
  );

  return $form;
}

/**
 * Helper function; sorts components by weight.
 */
function _flexifilter_components_sort_weight($a, $b) {
  return ((float)$a['weight']) > ((float)$b['weight']);
}

/**
 * Generates an FAPI array for a component list
 *
 * @param $data FAPI values for the component list. Use flexifilter_filter_edit_components_data to
 * convert a components array into coresponding FAPI values.
 * @param $data_base FAPI values for the top-level component list
 */
function flexifilter_filter_edit_form_components($data, &$data_base, $show_advanced) {
  $form = array(
    '#type' => 'fieldset',
    '#title' => t('Components'),
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
    '#tree' => TRUE,
  );
  $components = array();
  foreach ($data as $key => $component) {
    if (strncmp($key, $data_base['id_prefix'], strlen($data_base['id_prefix'])) == 0) {
      $components[$key] = $component;
    }
  }
  if (isset($data['op'])) {
    $components[$data_base['id_prefix'] . $data_base['id_next']] = flexifilter_filter_edit_form_new_component_data($data['new_list']);
    $data_base['id_next'] = $data_base['id_next'] + 1;
  }
  foreach ($components as $key => $component) {
    if (isset($component['op'])) {
      switch ($component['move']) {
        /*
          Careful; this component is NOT VALIDATED by the FAPI. We only act upon certain
          values ('top', 'up', 'down', 'bottom', 'remove'), which are always valid (even
          if they have no effect). Other values are silently ignored.
        */

        case 'top':
          $components[$key]['weight'] = -1000;
          break;

        case 'up':
          $components[$key]['weight'] = $component['weight'] - 1.5;
          break;

        case 'down':
          $components[$key]['weight'] = $component['weight'] + 1.5;
          break;

        case 'bottom':
          $components[$key]['weight'] = 10001;
          break;

        case 'remove':
          unset($components[$key]);
          break;
      }
      $form['#collapsed'] = FALSE;
    }
  }
  uasort($components, '_flexifilter_components_sort_weight');
  $new_weight = 0;
  $last_component_weight = count($components);
  foreach ($components as $key => $component) {
    $new_weight = $new_weight + 1;
    $form[$key] = flexifilter_filter_edit_form_component($component, $new_weight, $new_weight == 1, $new_weight == $last_component_weight, $data_base, $show_advanced);
    if ($form[$key]['#collapsed'] == FALSE) {
      $form['#collapsed'] = FALSE;
    }
  }
  $form['new_list'] = array(
    '#type' => 'select',
    '#options' => flexifilter_get_grouped_labels(flexifilter_get_component_list($show_advanced)),
    '#title' => t('Add component'),
  );
  $form['op'] = array(
    '#type' => 'submit',
    '#value' => t('Add'),
    '#process' => array('flexifilter_undo_submit_default_name', 'form_expand_ahah'),
  );
  return $form;
}

/**
 * The add/edit form for flexifilters.
 */
function flexifilter_filter_edit_form($form_state, $flexifilter = 'new') {
  if (!isset($form_state['post']) || !isset($form_state['post']['fid'])) {
    $data = array();
    $data['fid'] = $flexifilter === 'new' ? 'new' : $flexifilter['id'];
    $data['basic'] = array();
    $data['basic']['label'] = $flexifilter === 'new' ? '' : $flexifilter['label'];
    $data['basic']['description'] = $flexifilter === 'new' ? '' : $flexifilter['description'];
    $data['basic']['advanced'] = $flexifilter === 'new' ? 0 : $flexifilter['advanced'];
    $data['basic']['cache'] = $flexifilter === 'new' ? 1 : $flexifilter['cache'];
    $data['components'] = $flexifilter === 'new' ? array() : flexifilter_filter_edit_components_data($flexifilter['components']);
    if (!isset($data['components']['id_prefix']) || $data['components']['id_prefix'] === 'component_new_') {
      $data['components']['id_prefix'] = 'component_'. $data['fid'] .'_';
    }
    if (!isset($data['components']['id_next'])) {
      $data['components']['id_next'] = 0;
    }
    $expand_root_levels = TRUE;
  }
  else {
    $data = $form_state['post'];
    if (!isset($data['basic']['advanced'])) {
      $data['basic']['advanced'] = 0;
    }
    if (!isset($data['basic']['cache'])) {
      $data['basic']['cache'] = $flexifilter === 'new' ? 1 : $flexifilter['cache'];
    }
  }
  $form = array();
  $form['fid'] = array(
    '#type' => 'hidden',
    '#value' => $data['fid'],
  );
  $form['basic'] = array(
    '#type' => 'fieldset',
    '#title' => t('Basic settings'),
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
    '#tree' => TRUE,
  );
  $form['basic']['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Label'),
    '#required' => TRUE,
    '#maxlength' => 127,
    '#description' => t('Choose an appropriate label for this flexifilter.'),
    '#default_value' => $data['basic']['label'],
  );
  $form['basic']['description'] = array(
    '#type' => 'textfield',
    '#title' => t('Description/filter tips'),
    '#required' => TRUE,
    '#maxlength' => 255,
    '#description' => t('The tips to present to the user under the input format\'s description on the content creation page. The delimiter &lt;!--break--&gt;
	                     may be added to separate the short description from the long description.'),
    '#default_value' => $data['basic']['description'],
  );
  $form['basic']['advanced'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show advanced options'),
    '#default_value' => $data['basic']['advanced'],
    '#description' => t('Advanced options currently only include the processing step option, which allows you to place some components into the filtering preparation step. You should only do so if it is critical that they be performed in the preparation step rather than the processing step.'),
  );
  if ($data['basic']['advanced']) {
    $form['basic']['cache'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable caching'),
      '#description' => t('If caching is not enabled, the filter will run each time the content is viewed. It is useful for dynamic filters (e.g. things that change each time) to have this option off, but this can slow down page loads noticably.'),
      '#default_value' => $data['basic']['cache'],
    );
  }
  $form['components'] = flexifilter_filter_edit_form_components($data['components'], $data['components'], $data['basic']['advanced'] == 1);
  $form['components']['id_prefix'] = array('#type' => 'hidden', '#value' => $data['components']['id_prefix']);
  $form['components']['id_next'] = array('#type' => 'hidden', '#value' => $data['components']['id_next']);
  if (isset($expand_root_levels) && $expand_root_levels) {
    $form['basic']['#collapsed'] = FALSE;
    $form['components']['#collapsed'] = FALSE;
  }
  $form['submit1'] = array(
    '#type' => 'submit',
    '#value' => $data['fid'] === 'new' ? t('Create') : t('Save'),
  );
  $form['submit2'] = array(
    '#type' => 'submit',
    '#value' => $data['fid'] === 'new' ? t('Create and edit') : t('Save and edit'),
  );
  return $form;
}

/**
 * Helper function for flexifilter_filter_edit_form_components_from_data; converts a condition's
 * FAPI values back into a condition array.
 */
function flexifilter_filter_edit_form_condition_from_data($data) {
  $condition = array(
    'class' => $data['class'],
    'settings' => array(),
  );
  foreach ($data as $key => $value) {
    if (strncmp($key, 'setting_', 8) == 0) {
      $condition['settings'][substr($key, 8)] = $value;
    }
    elseif (strncmp($key, 'condition_', 10) == 0) {
      if (!isset($condition['conditions'])) {
        $condition['settings']['conditions'] = array();
      }
      $condition['settings']['conditions'][] = flexifilter_filter_edit_form_condition_from_data($value);
    }
  }
  return $condition;
}

/**
 * Helper function for flexifilter_filter_edit_form_submit; converts components FAPI values
 * back into a components array.
 */
function flexifilter_filter_edit_form_components_from_data($data, $data_base = NULL) {
  $components = array();
  if (!isset($data_base)) {
    $data_base = $data;
    $components['id_prefix'] = $data_base['id_prefix'];
    $components['id_next'] = $data_base['id_next'];
  }
  foreach ($data as $key => $data_us) {
    if (strncmp($key, $data_base['id_prefix'], strlen($data_base['id_prefix'])) == 0) {
      $component = array(
        'class' => $data_us['class'],
        'settings' => array(),
      );
      foreach ($data_us as $key => $value) {
        if (strncmp($key, 'setting_', 8) == 0) {
          $component['settings'][substr($key, 8)] = $value;
        }
      }
      if (isset($data_us['step'])) {
        $component['settings']['step'] = $data_us['step'];
      }
      if (isset($data_us['components'])) {
        $component['settings']['components'] = flexifilter_filter_edit_form_components_from_data($data_us['components'], $data_base);
      }
      if (isset($data_us['condition'])) {
        $component['settings']['condition'] = flexifilter_filter_edit_form_condition_from_data($data_us['condition']);
      }
      $components[] = $component;
    }
  }
  return $components;
}

function flexifilter_filter_edit_form_submit_components($data, $fid, &$pids_to_reuse, $parent = FALSE) {
  if ($parent === FALSE) {
    if ($reuse_pid = array_shift($pids_to_reuse)) {
      db_query("UPDATE {flexifilters_parts} SET fid = %d, parent_pid = 0, type = %d, class_name = '', settings = '' WHERE pid = %d",
        $fid, FLEXIFILTER_PART_TYPE_ROOT, $reuse_pid);
      $parent = $reuse_pid;
    }
    else {
      db_query("INSERT INTO {flexifilters_parts} (fid, parent_pid, type, class_name, settings) VALUES (%d, 0, %d, '', '')",
        $fid, FLEXIFILTER_PART_TYPE_ROOT);
      $parent = db_last_insert_id('flexifilters_parts', 'pid');
    }
  }
  foreach ($data as $key => $child) {
    if (is_numeric($key)) {
      if ($child_cid = array_shift($pids_to_reuse)) {
        db_query("UPDATE {flexifilters_parts} SET fid = %d, parent_pid = %d, type = %d, class_name = '%s', settings = '%s' WHERE pid = %d",
          $fid, $parent, FLEXIFILTER_PART_TYPE_COMPONENT, $child['class'], serialize($child['settings']), $child_cid);
      }
      else {
        db_query("INSERT INTO {flexifilters_parts} (fid, parent_pid, type, class_name, settings) VALUES (%d, %d, %d, '%s', '%s')",
          $fid, $parent, FLEXIFILTER_PART_TYPE_COMPONENT, $child['class'], serialize($child['settings']));
        $child_cid = db_last_insert_id('flexifilters_parts', 'pid');
      }
      if (isset($child['components'])) {
        flexifilter_filter_edit_form_submit_components($child['components'], $fid, $pids_to_reuse, $child_cid);
      }
      if (isset($child['condition'])) {
        flexifilter_filter_edit_form_submit_condition($child['condition'], $fid, $pids_to_reuse, $child_cid);
      }
    }
  }
  return $parent;
}

function flexifilter_filter_edit_form_submit_condition($data, $fid, &$pids_to_reuse, $parent) {
  if ($our_pid = array_shift($pids_to_reuse)) {
    db_query("UPDATE {flexifilters_parts} SET fid = %d, parent_pid = %d, type = %d, class_name = '%s', settings = '%s' WHERE pid = %d",
      $fid, $parent, FLEXIFILTER_PART_TYPE_CONDITION, $data['class'], serialize($data['settings']), $our_pid);
  }
  else {
    db_query("INSERT INTO {flexifilters_parts} (fid, parent_pid, type, class_name, settings) VALUES (%d, %d, %d, '%s', '%s')",
      $fid, $parent, FLEXIFILTER_PART_TYPE_CONDITION, $data['class'], serialize($data['settings']));
    $our_pid = db_last_insert_id('flexifilters_parts', 'pid');
  }
  if (isset($data['conditions'])) {
    foreach ($data['conditions'] as $condition) {
      flexifilter_filter_edit_form_submit_condition($condition, $fid, $pids_to_reuse, $our_pid);
    }
  }
}

function flexifilter_filter_edit_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $values['components'] = flexifilter_filter_edit_form_components_from_data($values['components']);
  $values += $values['basic'];
  unset($values['basic']);
  if ($values['fid'] === 'new') {
    $enabled = 0;
    if (flexifilter_get_number_enabled_filters() < FLEXIFILTER_MAX_FILTERS) {
      $enabled = 1;
    }
    $values['enabled'] = $enabled;
  }
  $fid = flexifilter_save_filter($values);
  if ($values['fid'] === 'new') {
    drupal_set_message(t('Flexifilter created.'));
    if ($values['op'] == $values['submit2']) {
      unset($form_state['storage']);
      unset($form_state['rebuild']);
      $form_state['redirect'] = 'admin/build/flexifilters/'. $fid .'/edit';
    }
  }
  else {
    drupal_set_message(t('Flexifilter saved.'));
  }
  if ($values['op'] == $values['submit1']) {
    unset($form_state['storage']);
    unset($form_state['rebuild']);
    $form_state['redirect'] = 'admin/build/flexifilters';
  }
}

function flexifilter_filter_delete_form($form_state, $flexifilter) {
  $form = array();
  $form['fid'] = array('#type' => 'hidden', '#value' => $flexifilter['id']);

  return confirm_form($form, t('Are you sure you want to delete the flexifilter "%flexifilter"?', array('%flexifilter' => $flexifilter['label'])), 'admin/build/flexifilters', t('This action cannot be undone.'), t('Delete'), t('Cancel'));
}

function flexifilter_remove_from_filters($delta) {
  db_query("DELETE FROM {filters} WHERE module = 'flexifilter' AND delta = %d", $delta);
  cache_clear_all('*', 'cache_filter', TRUE);
}

function flexifilter_filter_delete_form_submit($form, &$form_state) {
  $fid     = $form_state['values']['fid'];
  $filters = flexifilter_get_filters();
  $filter  = $filters[$fid];
  if ($filter['enabled']) {
    flexifilter_remove_from_filters($filter['delta']);
  }
  db_query('DELETE FROM {flexifilters_parts} WHERE fid = %d', $fid);
  db_query('DELETE FROM {flexifilters} WHERE fid = %d', $fid);
  drupal_set_message(t('Flexifilter deleted.'));
  $form_state['redirect'] = 'admin/build/flexifilters';
}

function flexifilter_filter_disable_form($form_state, $flexifilter) {
  $form = array();
  $form['fid'] = array('#type' => 'hidden', '#value' => $flexifilter['id']);

  return confirm_form($form, t('Are you sure you want to disable the flexifilter "%flexifilter"?', array('%flexifilter' => $flexifilter['label'])), 'admin/build/flexifilters', t('If you disable the flexifilter, then it cannot be used in any Input Formats.'), t('Disable'), t('Cancel'));
}

function flexifilter_filter_disable_form_submit($form, &$form_state) {
  $fid     = $form_state['values']['fid'];
  $filters = flexifilter_get_filters();
  $filter  = $filters[$fid];
  if ($filter['enabled']) {
    flexifilter_remove_from_filters($filter['delta']);
    db_query('UPDATE {flexifilters} SET enabled = 0 WHERE fid = %d', $fid);
    drupal_set_message(t('Flexifilter disabled.'));
  }
  $form_state['redirect'] = 'admin/build/flexifilters';
}

function flexifilter_filter_enable_form($form_state, $flexifilter) {
  $form = array();
  $form['fid'] = array('#type' => 'hidden', '#value' => $flexifilter['id']);

  return confirm_form($form, t('Are you sure you want to enable the flexifilter "%flexifilter"?', array('%flexifilter' => $flexifilter['label'])), 'admin/build/flexifilters', t(''), t('Enable'), t('Cancel'));
}

function flexifilter_filter_enable_form_submit($form, &$form_state) {
  $fid     = $form_state['values']['fid'];
  $filters = flexifilter_get_filters();
  $filter  = $filters[$fid];
  if (!$filter['enabled'] && flexifilter_get_number_enabled_filters() < FLEXIFILTER_MAX_FILTERS) {
    db_query('UPDATE {flexifilters} SET enabled = 1, delta = %d WHERE fid = %d', flexifilter_get_unused_delta(), $fid);
    drupal_set_message(t('Flexifilter enabled.'));
  }
  $form_state['redirect'] = 'admin/build/flexifilters';
}

function flexifilter_filter_export_form($form_state, $flexifilter) {
  $form = array();
  $flexifilter['fid'] = 'new';
  $form['serialized'] = array(
    '#type' => 'textarea',
    '#attributes' => array('readonly' => 'readonly'),
    '#cols' => '100',
    '#title' => t('Serialized flexifilter (for manual imports)'),
    '#default_value' => serialize($flexifilter),
  );
  $form['flexifilter'] = array(
    '#type' => 'textarea',
    '#attributes' => array('readonly' => 'readonly'),
    '#cols' => '100',
    '#title' => t('Exported flexifilter (for hook_flexifilters implementations)'),
    '#default_value' => var_export($flexifilter, TRUE),
  );
  return $form;
}

function flexifilter_filter_input_form($form_state) {
  $form = array();
  $form['flexifilter'] = array(
    '#type' => 'textarea',
    '#title' => t('Flexifilter to import'),
    '#description' => t('Enter the serialized flexifilter as from a flexifilter export here.'),
    '#default_value' => '',
    '#required' => TRUE,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import'),
  );
  return $form;
}

function flexifilter_filter_input_form_submit($form, &$form_state) { 
  $fid = unserialize($form_state['values']['flexifilter']);
  if ($fid) {
    drupal_goto('admin/build/flexifilters/'. flexifilter_save_filter($form_state['values']['flexifilter']) .'/edit');
  }
  else {
    drupal_set_message(t('It appears that the serialized flexifilter you entered was improperly formed. Please check the source.'), 'error');
  }
}

function flexifilter_filter_default_form($form_state) {
  $flexifilters = module_invoke_all('flexifilters');
  $names = array();
  foreach ($flexifilters as $key => $flexifilter) {
    $names[$key] = $flexifilter['label'];
  }
  $form['selections'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Default flexifilters to load'),
    '#multiple' => TRUE,
    '#options' => $names,
    '#required' => TRUE,
    '#default_value' => array(),
  );
  $form['flexifilters'] = array(
    '#type' => 'value',
    '#value' => $flexifilters,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Load'),
  );
  return $form;
}

function flexifilter_filter_default_form_submit($form, &$form_state) {
  $flexifilters = $form_state['values']['flexifilters'];
  $selections = $form_state['values']['selections'];
  $to_be_saved = array();
  foreach ($selections as $selection) {
    if (isset($flexifilters[$selection])) {
      $to_be_saved[] = $flexifilters[$selection];
    }
  }
  flexifilter_install_flexifilters('flexifilter', $to_be_saved);
  $form_state['redirect'] = 'admin/build/flexifilters';
}

function flexifilter_filter_preview_form($form_state, $flexifilter) {
  if (!isset($form_state['storage']['preview'])) {
    $form['input'] = array(
      '#type' => 'textarea',
      '#title' => t('Text to test'),
      '#default_value' => '',
    );
    $form['flexifilter'] = array(
      '#type' => 'value',
      '#value' => $flexifilter,
    );
    $form['submit'] = array(
     '#type' => 'submit',
      '#value' => t('Preview'),
    );
  }
  else {
    $form['input'] = array(
      '#type' => 'textarea',
      '#title' => t('Original text'),
      '#default_value' => $form_state['storage']['input'],
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Preview'),
    );
    $form['prepare'] = array(
      '#type' => 'fieldset',
      '#title' => t('Prepare step'),
      '#collapsible' => TRUE,
    );
    $form['process'] = array(
      '#type' => 'fieldset',
      '#title' => t('Process step'),
      '#collapsible' => TRUE,
    );
    $form['flexifilter'] = array(
      '#type' => 'value',
      '#value' => $flexifilter,
    );
    $zebra = 'flexifilter-even';
    drupal_add_css(drupal_get_path('module', 'flexifilter') .'/flexifilter.css');
    foreach ($form_state['storage']['data'] as $key => $data) {
      $settings = '';
      foreach ($data['settings'] as $key => $value) {
        $settings .= '<strong>'. check_plain($key) .'</strong>: '. check_plain(''. $value) ."<br />";
      }
      $uniqid = uniqid();
      $form[$data['step']][$key . $uniqid]['step'] = array(
        '#type' => 'markup',
        '#prefix' => '<div class="'. $zebra .'"><div class="description">',
        '#value' => t('@type: @class<p>Settings:<br />!settings</p>', array('@type' => ucfirst($data['type']), '@class' => $data['class'], '!settings' => $settings)),
        '#suffix' => '</div>',
      );
      $form[$data['step']][$key . $uniqid]['text'] = array(
        '#type' => 'markup',
        '#title' => ucfirst($data['type']) .': '. $data['class'],
        '#description' => t('Settings:<br />!settings', array('!settings' => $settings)),
        '#value' => $data['value'] . '</div><strong>'. t('With visible HTML:') .'</strong><div class="flexifilter-plain-text">'. check_plain($data['value']),
        '#prefix' => '<strong>'. t('As it appears to users:') .'</strong><div class="flexifilter-text">',
        '#suffix' => '</div></div>',
      );
      $zebra = ($zebra == 'flexifilter-even') ? 'flexifilter-odd' : 'flexifilter-even';
    }
  }
  return $form;
}

function flexifilter_filter_preview_form_submit($form, &$form_state) {
  variable_set('flexifilter_preview', TRUE);
  variable_set('flexifilter_preview_text', array());
  $form_state['storage']['input'] = $form_state['values']['input'];
  $text = flexifilter_invoke_components($form_state['values']['flexifilter']['components'], 'prepare', $form_state['values']['input']);
  flexifilter_invoke_components($form_state['values']['flexifilter']['components'], 'process', $text);
  $form_state['storage']['preview'] = TRUE;
  $form_state['storage']['data'] = (variable_get('flexifilter_preview_text', array()));
  variable_del('flexifilter_preview');
  variable_del('flexifilter_preview_text');
}